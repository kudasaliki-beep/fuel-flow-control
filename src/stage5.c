/* Stage 5
 */

#include <stdio.h>

#define dt 0.1 /*ms*/
#define CEILING      3000.0   /* MJ/h, F1 2026 energy flow limit */

double apply_fuel_limiter(double driver_demand, double mass_flow_limit)

{
    if (driver_demand > mass_flow_limit)
        return mass_flow_limit;

    else
     return driver_demand;  
}

int main(void)
{
double energy_flow, uncontrolled_energy_flow, ceiling_comparison, driver_demand, x, elapsed_time, mass_flow_limit, actual_mass_flow ;



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
  double step_time;
  char name [50];
};

struct test_scenario1 scenario = { 0, 80, 5, "corner_exit, standard" };

mass_flow_limit = CEILING / team.fuel_energy_density; /* PLANT = energy_flow = actual_mass_flow * team.fuel_energy_density*/

FILE *pF = fopen ("results.csv", "w");

if (pF == NULL) {
    printf("Error: could not open file.\n");
    return 1;
}


fprintf(pF,"elapsed_time,driver_demand,actual_mass_flow,energy_flow, uncontrolled_energy_flow, ceiling_comparison,mass_flow_limit\n");
for (x = 0; x < 200; x++) /* x = iterations, 1 iteration is 0.1ms */

   { 
    elapsed_time = x * dt;
    
    if ( elapsed_time < scenario.step_time) driver_demand = scenario.before_value; else driver_demand = scenario.after_value; /* driver demands 80 kg/h of fuel*/


actual_mass_flow = apply_fuel_limiter(driver_demand, mass_flow_limit); /* is the drivers request accepted or ignored? - uses function*/
energy_flow = actual_mass_flow * team.fuel_energy_density;

uncontrolled_energy_flow = driver_demand * team.fuel_energy_density;

ceiling_comparison = energy_flow - CEILING;

printf("driver demand: %fkg/h\n actual mass flow: %fkg/h\n" " energy flow: %fMJ/h\n ceiling comparison: %fMJ/h\n"  " elapsed time: %fms\n\n", driver_demand, actual_mass_flow, energy_flow, ceiling_comparison, elapsed_time);
fprintf(pF, "%f,%f,%f,%f,%f,%f,%f\n",elapsed_time,driver_demand,actual_mass_flow,energy_flow, uncontrolled_energy_flow, ceiling_comparison,mass_flow_limit);
    }

 fclose(pF);  



    return 0;
}