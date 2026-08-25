
/* Stage 2
 */

#include <stdio.h>

#define dt 0.1 /*ms*/
#define ENERGY_DENSITY 42.0   /* MJ/kg, constant */
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
double energy_flow, ceiling_comparison, driver_demand, x, elapsed_time, mass_flow_limit, actual_mass_flow ;
 


mass_flow_limit = CEILING / ENERGY_DENSITY; 

for (x = 0; x < 200; x++) /* x = iterations, 1 iteration is 0.1ms */

   { 
    elapsed_time = x * dt;
    
    if (elapsed_time < 5) driver_demand = 0; else driver_demand = 80; /* driver demands 80 kg/h of fuel*/

    
actual_mass_flow = apply_fuel_limiter(driver_demand, mass_flow_limit); /* is the drivers request accepted or ignored - uses function*/
energy_flow = actual_mass_flow * ENERGY_DENSITY;
ceiling_comparison = energy_flow - CEILING;

printf("driver demand: %fkg/h\n actual mass flow: %fkg/h\n" " energy flow: %fMJ/h\n ceiling comparison: %fMJ/h\n"  " elapsed time: %fms\n\n", driver_demand, actual_mass_flow, energy_flow, ceiling_comparison, elapsed_time);
    }



    return 0;
}
