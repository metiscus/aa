#include "core.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

/*
	References:
	[1] - A simple dynamic model for the primary circuit in VVER
	      plants for controller design purposes. - Fazekas
*/

namespace Constants
{
	double RodParams [3] = { -1.322e-4, -6.08e-5, -2.85e-4 }; // { m^2, m, dimensionless }

	double Lambda  = 10e-5;	// 
	double S       = 2859.0;	// %/s
	double CpPC    = 5281.0;	// J/kg/K
	double KtSG1   = 9.19e6;	// W/K
	double KlossPC = 3.0e6;		// W/K
	double alpha   = 1.097;		// --
	double Msg0    = 31810.0;	// kg
	double CpSG    = 4651.1;	// J/kg/K
	double KlossSG = 1.52e8;	// W
	double Ktsg2   = 3.30e6;	// W/K
	double beta    = 2.004;		// --
	double CpWMw   = 2.031e7;	// J/K
	double Tw0     = 267.9;		// deg C
	double CpPR    = 5895.4;  	// J/kg/K
	double WlossPR = 1.48e5;   	// W
	double Cpsi    = 13.75e6;   // W / %
	double Apr     = 4.52;		// m^2
	double V0pc    = 242.0;     // m^3
	double mout    = 2.9722;    // kg/s
	double min     = 1.4222;    // kg/s
	double TpcLoss = 10.0;		// temperature loss between Tpc,CL and Tpci
	double ToutPC  = 293.0;
	double Msg     = 120.56;	// mass of steam / water through steam-generator
	double Wr      = 13.75e8;   // W

#if 0
	double iodine_fission_yield = 6.2e-2;
	//double iodine_fission_yield = 1.8e-3;
	double iodine_fission_yield_sq = 0.0;
#else
	double iodine_fission_yield = 1e-4;
	//double iodine_fission_yield = 1.8e-3;
	double iodine_fission_yield_sq = 1e-5;
#endif
	double iodine_half_life = 23760.0;
	double xenon_fission_yield  = 1.22e-2;
	double xenon_capture_probability = 2.65e-7; // per second
	//double xenon_capture_probability = 2.65e-3; // per second

	double iodine_capture_probability =  8e-3 * xenon_capture_probability; // per second
	//double xenon_capture_probability = 2.65e-2; // per second

	constexpr double neutron_scale = 1e4;

	static constexpr double BARToPsi = 14.504;
}

namespace
{
	// Estimates the water density at a given temperature kelvin
	inline double water_density(double temperature)
	{
		constexpr double C0 = 581.2;
		constexpr double C1 = 2.98;
		constexpr double C2 = -0.00848;

		return C0 + C1 * temperature + C2 * temperature * temperature;
	}

	// Estimates the saturated vapor pressure using centigrade
	// returns in units of kPa
	inline double saturated_vapor_pressure(double temperature)
	{
		return 28884.78 - 258.01 * temperature + 0.63 * temperature * temperature;
	}

	inline double my_pow(double x, double y)
	{
		if(x < 0.0f)
		{
			return -1.0 * pow(-x, y);
		}
		else
		{
			return pow(x,y);
		}
	}

	inline double compute_water_boil_temperature(double pressure)
	{
		//const double hvap = 40.66; //kj/mol
		//const double T0 = 100.0;
		//const double P0 = 101.325e3; // 1 atm
		//const double R = 8.3144589e-5;
		//double boiling_point = 1.0 / ((1.0 / T0) - (R*log((pressure * PSIToPa)/P0)) / hvap);
		static constexpr double PSIToKPa = 6.895;
		double boiling_point = log((pressure * PSIToKPa) / 66.5238) / 0.015758;
		return boiling_point;
	}
}

Core::Core()
	: rod_position_(0.0)
	, flux_(1.0)
	, primary_loop_coolant_mass_(180600.0)
	, primary_loop_coolant_temperature_(20.0)
	, primary_loop_coolant_pressure_(123.0 * Constants::BARToPsi)
	, primary_loop_coolant_volume_(242)
{

}

void Core::simulate(double dt)
{
	static double timebank = 0.0;
	timebank += dt;

	constexpr double FixedTimestep = 1.0 / 60.0;

	while(timebank > FixedTimestep)
	{
		timebank -= FixedTimestep;

		
		// Integrate the neutron flux in the core
		double dFlux = (1.0 / Constants::Lambda) * (Constants::RodParams[0] * rod_position_ * rod_position_ + Constants::RodParams[1] * rod_position_ + Constants::RodParams[2]) * flux_ + Constants::S;
		
		flux_ = flux_ + dFlux * FixedTimestep;

		if(flux_ <= 0.0)
		{
			flux_ = 0.0;
		}

		// Thermal output of the reactor core
		double thermal_output = flux_ * 1e-3 * Constants::Wr;
		double joules_transferred = thermal_output * FixedTimestep;
		double water_temperature_difference = joules_transferred / (primary_loop_coolant_mass_ * Constants::CpPC);
		

		fprintf(stderr, "Water tmp: %lf xnge: %lf  %lf\n", primary_loop_coolant_temperature_, water_temperature_difference, compute_water_boil_temperature(primary_loop_coolant_pressure_));

		primary_loop_coolant_temperature_ += water_temperature_difference;
		double water_boil_temp = compute_water_boil_temperature(primary_loop_coolant_pressure_);

	}
}