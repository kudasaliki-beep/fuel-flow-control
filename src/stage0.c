/* Stage 0: single time-step calculation.
 * Derives energy flow from mass flow x energy density and compares
 * it against the F1 2026 fuel energy flow ceiling (3000 MJ/h).
 * See README.md for assumptions and design decisions at this stage. */

#include <stdio.h>

#define MASS_FLOW      70.0   /* kg/h, test value */
#define ENERGY_DENSITY 42.0   /* MJ/kg, test value */
#define CEILING      3000.0   /* MJ/h, F1 2026 energy flow limit */

int main(void)
{
    double energy_flow, ceiling_comparison;

    energy_flow = MASS_FLOW * ENERGY_DENSITY;
    printf("Energy flow rate: %f MJ/h\n", energy_flow);

    ceiling_comparison = energy_flow - CEILING;
    printf("Ceiling comparison: %f MJ/h (negative = under limit)\n",
           ceiling_comparison);

    return 0;
}
