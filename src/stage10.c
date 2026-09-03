/* Stage 10
simulation of a driver requesting 80kg/h of fuel during a corner exit, with an energy limit 0f 3000 MJ/h

●	Source: FastF1 Python library, reading the FIA public timing/telemetry feed
●	Session: 2023 Monaco Grand Prix, 
●	Lap 7, Turn 7 (Portier), tight right-hander leading onto the tunnel straight.
*/

#include <stdio.h>
#include <math.h>

#define tau 4 /*ms*/


#define dt 0.1 /*ms*/
#define CEILING      3000.0   /* MJ/h, F1 2026 energy flow limit */


/*Tuneable Kp Ki Kd constants*/
#define Kp 0.3
#define Ki 0.05 /*actual integral term is Ki × integral */
#define Kd 0.0

double apply_fuel_limiter(double requested_mass_flow, double mass_flow_limit)
{
    if (requested_mass_flow > mass_flow_limit)
        return mass_flow_limit;
    else
        return requested_mass_flow;
}

int main(void)
{
double candidate_mass_flow, derivative, pid_output, error, previous_error, integral, previous_actual_mass_flow, buffer[30], torque_delayed, 
lagged_demand, energy_flow, uncontrolled_energy_flow, 
ceiling_comparison, driver_demand, x, elapsed_time, mass_flow_limit, actual_mass_flow ;

int i;


double rpm, dynamic_ceiling;


struct team
{
  double fuel_energy_density;
  char name [50];
};

struct team team;

/*fuel density can now be grouped with a specific team*/
printf("Enter team name:");
scanf("%s", team.name);

printf("Enter fuel density:");
scanf("%lf", &team.fuel_energy_density);





struct test_scenario1
{
  double  before_value;
  double after_value; 
  double tmid;
  char name [50];
};

/*t_mid is the timestamp (in milliseconds) at the center of sigmoid transition*/

struct test_scenario1 scenario = { 0, 80, 175, "corner_exit, standard" };

/* options for future test scenarios - struct test_scenario1 etc*/

FILE *pF = fopen ("results.csv", "w");

if (pF == NULL) {
    printf("Error: could not open file.\n");
    return 1;
}


fprintf(pF,"elapsed_time,driver_demand,lagged_demand,torque_delayed,pid_output,candidate_mass_flow,actual_mass_flow,energy_flow,uncontrolled_energy_flow,ceiling_comparison,mass_flow_limit,rpm,dynamic_ceiling\n");

integral = 0; /* integral starts at 0*/
previous_error = 0;

rpm = 4000;

previous_actual_mass_flow = scenario.before_value;

for (i = 0; i < 30; i++) {
    buffer[i] = scenario.before_value;
}

/* lagged demand - the fuel system's physical inability to instantly reach the position the ECU has commanded, the valve (actuator) gradually catching up to the current driver demand.*/
lagged_demand = scenario.before_value;
for (x = 0; x < 4500; x++) /* x = iterations, 1 iteration is 0.1ms */

   { 
    elapsed_time = x * dt;
    
    driver_demand = scenario.before_value + (scenario.after_value - scenario.before_value) / (1 + exp(-0.067 * (elapsed_time - scenario.tmid))); /* sigmoid driver demand transition from before_value to after_value */
   
 lagged_demand = lagged_demand + (dt / tau) * (driver_demand - lagged_demand); 

rpm = rpm + (2080.3 * (driver_demand / 80.0)) * (dt / 1000.0);

 /*CALCULATE THE DYNAMIC 2026 FIA ENERGY CEILING*/
    if (rpm < 10500.0) {
        dynamic_ceiling = (0.27 * rpm) + 165.0; 
    } else {
        dynamic_ceiling = CEILING;               
    }

mass_flow_limit = dynamic_ceiling / team.fuel_energy_density;

/* torque delay - air travel, fuel mixing, combustion, and mechanical force transfer downstream of the actuator; buffer holds a rolling history of lagged_demand, delayed by BUFFER_SIZE iterations. 
lagged_demand feeds directly into torque_delayed: it's the exact same value experienced 30 iterations later(3ms, 3/dt or 3/0.1 = 30 iterations). It's simply a timeshift of lagged_demand so is the new result of driver demand. As for the value 3ms I just picked 3ms as researched torque delay tends to be 2-5ms*/

/* in other words torque delayed is just driver demand after actuator lag and after the other delays just mentioned (air travel etc)*/

torque_delayed = buffer[30 - 1];              /* 1. read the oldest, before anything gets overwritten */

for (i = 30 - 1; i > 0; i--) {                /* 2. shift everything one slot toward the "older" end */
    buffer[i] = buffer[i - 1];
}

buffer[0] = lagged_demand;                      /* 3. write  fresh value into the newest slot */


error = mass_flow_limit - previous_actual_mass_flow; /*error signal for PID controller*/
derivative = (error - previous_error) / dt;
pid_output = (Kp * error) + (Ki * integral) + (Kd * derivative);

if (pid_output <= torque_delayed) {
    integral = integral + error * dt;
}                                         

candidate_mass_flow = apply_fuel_limiter(torque_delayed, pid_output); /*uses apply fuel limiter function to compare torque delayed with pid output, lower value becomes candidate mass flow*/
actual_mass_flow = apply_fuel_limiter(candidate_mass_flow, mass_flow_limit); /* compares the candidate mass flow against the ahrd ceiling, safety net incase PID produces something over the hard limit (overshoot, bad tuning)*/

energy_flow = actual_mass_flow * team.fuel_energy_density;

uncontrolled_energy_flow = driver_demand * team.fuel_energy_density;

ceiling_comparison = energy_flow - dynamic_ceiling;

printf("driver demand: %fkg/h\n actual mass flow: %fkg/h\n" " energy flow: %fMJ/h\n ceiling comparison: %fMJ/h\n"  " elapsed time: %fms\n\n", driver_demand, actual_mass_flow, energy_flow, ceiling_comparison, elapsed_time); /* prints to terminal only*/
fprintf(pF, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",elapsed_time,driver_demand,lagged_demand,torque_delayed,pid_output,candidate_mass_flow,actual_mass_flow,energy_flow,uncontrolled_energy_flow,ceiling_comparison,mass_flow_limit,rpm,dynamic_ceiling);
    
previous_actual_mass_flow = actual_mass_flow;
previous_error = error;

}

 fclose(pF);  



    return 0;
}