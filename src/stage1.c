
/* Stage 1
 */

#include <stdio.h>

#define dt 0.1 /*ms*/
#define ENERGY_DENSITY 42.0   /* MJ/kg, constant */
#define CEILING      3000.0   /* MJ/h, F1 2026 energy flow limit */

int main(void)
{
    double energy_flow, ceiling_comparison, mass_flow, x, elapsed_time;



    for (x = 0; x < 200; x++) /* x = iterations, 1 iteration is 0.1ms */

   { 
    elapsed_time = x * dt;
    
if (elapsed_time < 5) mass_flow = 0; else mass_flow = 80;
energy_flow = mass_flow * ENERGY_DENSITY;

ceiling_comparison = energy_flow - CEILING;

printf (" mass flow: %fkg/h\n energy flow: %fMj/h\n ceiling comparison %fMj/h\n elapsed time %fms\n", mass_flow, energy_flow, ceiling_comparison,elapsed_time);
}

    return 0;
}
