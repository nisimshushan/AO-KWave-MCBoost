//
//  AO_sim.cpp
//  K_Wave_C
//
//  Created by jacob on 12/3/12.
//  Copyright (c) 2012 BMPI. All rights reserved.
//

#include <exception>

#include "AO_sim.h"
#include <MC-Boost/logger.h>





AO_Sim::AO_Sim()
{
    // Responsible for running the monte-carlo simulation.
    da_boost = new MC_Boost();
    
    // Responsible for running the k-Wave simulation.
    KSpaceSolver = new TKSpaceFirstOrder3DSolver();
    
    /// Set default values for private members.
    m_Laser_injection_coords.x = -1;
    m_Laser_injection_coords.y = -1;
    m_Laser_injection_coords.z = -1;

	/// Set default value.
	MC_time_step = 0.0f;
    
}



AO_Sim::~AO_Sim()
{
    cout << "\n\nAO_Sim:: destructor\n";
    if (da_boost)
        delete da_boost;
    
    if (KSpaceSolver)
        delete KSpaceSolver;
    
//    if (m_medium)
//        delete m_medium;
    
}



/// Run the actual acousto-optic simulation.
void
AO_Sim::Run_acousto_optics_sim(TParameters * Parameters,
							   bool sim_displacement,
							   bool sim_refractive_grad)
{
    
    
    /// Load data from disk
    cout << "Loading k-Wave data ........";
    try {
        KSpaceSolver->LoadInputData();
    }catch (ios::failure e) {
        cout << "Failed!\nK-Wave panic: Data loading was not successful!\n%s\n"
             << e.what();
        cerr << "K-Wave panic: Data loading was not successful! \n%s\n"
             << e.what();
        exit(EXIT_FAILURE);
    }
    cout << ".... done\n";
    
    //fprintf(stdout,"Elapsed time:          %8.2fs\n",KSpaceSolver.GetDataLoadTime());
    
    /// start computation of k-Wave simulation.
    cout << ".......... k-Wave Computation ...........\n";
    KSpaceSolver->PreCompute();
    
    
//    ActPercent = 0;
    KSpaceSolver->FromMain_PrintOutputHeader();
//
    KSpaceSolver->IterationTimeStart();
	size_t k_wave_Nt = Parameters->Get_Nt();
	//size_t k_wave_Nt = 4;
    for (KSpaceSolver->SetTimeIndex(0); KSpaceSolver->GetTimeIndex() < k_wave_Nt; KSpaceSolver->IncrementTimeIndex()){
        
        cout << ".......... Running k-Wave ........... ("
             << KSpaceSolver->GetTimeIndex() << " of "
             << Parameters->Get_Nt() << ")\n";

        KSpaceSolver->FromMain_Compute_uxyz();

	// add in the velocity u source term
        KSpaceSolver->FromMain_Add_u_source();

	// add in the transducer source term (t = t1) to ux
        if (Parameters->Get_transducer_source_flag() > KSpaceSolver->GetTimeIndex())
            KSpaceSolver->FromMain_AddTransducerSource();


        KSpaceSolver->FromMain_Compute_duxyz();


        if (Parameters->Get_nonlinear_flag())
            KSpaceSolver->FromMain_Compute_rhoxyz_nonlinear();
        else
            KSpaceSolver->FromMain_Compute_rhoxyz_linear();


	// add in the source pressure term
        KSpaceSolver->FromMain_Add_p_source();

        if (Parameters->Get_nonlinear_flag()){
            KSpaceSolver->FromMain_Compute_new_p_nonlinear();
        }else {
            KSpaceSolver->FromMain_Compute_new_p_linear();

        }


	//-- calculate initial pressure
        if ((KSpaceSolver->GetTimeIndex() == 0) && (Parameters->Get_p0_source_flag() == 1))
            KSpaceSolver->FromMain_Calculate_p0_source();

        
        /// --------------------------- Begin Monte-Carlo Simulation ------------------------------------------------------
        ///
		/// If the flag for simulating the influence of refractive index changes is set
		/// we need to grab the appropriate data from k-Wave and build grids for
		/// photon propagation to use in order to alter its path accordingly.

		TRealMatrix *currentPressure = &(KSpaceSolver->FromMain_Get_p());
        TRealMatrix *current_rhox    = &(KSpaceSolver->FromMain_Get_rhox());
       	TRealMatrix *current_rhoy    = &(KSpaceSolver->FromMain_Get_rhoy());
       	TRealMatrix *current_rhoz    = &(KSpaceSolver->FromMain_Get_rhoz());
       	TRealMatrix *rho0            = &(KSpaceSolver->FromMain_Get_rho0());
       	TRealMatrix *c2              = &(KSpaceSolver->FromMain_Get_c2());

		TRealMatrix *currentVelocity_Xaxis = &(KSpaceSolver->FromMain_Get_ux());
        TRealMatrix *currentVelocity_Yaxis = &(KSpaceSolver->FromMain_Get_uy());
        TRealMatrix *currentVelocity_Zaxis = &(KSpaceSolver->FromMain_Get_uz());

		/// NOTE:
		/// 	- These need to be updated along every time step of k-Wave to remain accurate.
		if (sim_refractive_grad &&
			sim_displacement)
		{
			/// Create a refractive map based upon the pressure at this time step.
        	m_medium->Create_refractive_map(currentPressure,
            	                            current_rhox,
            	                            current_rhoy,
            	                            current_rhoz,
            	                            rho0,
            	                            c2,
            	                            pezio_optical_coeff);

			/// Create a displacment map based upon the pressure at this time step.
        	m_medium->Create_displacement_map(currentVelocity_Xaxis,
            	                              currentVelocity_Yaxis,
            	                              currentVelocity_Zaxis,
            	                              m_medium->kwave.US_freq,
            	                              m_medium->kwave.dt);
		}
		else if (sim_refractive_grad)
		{
			/// Create a refractive map based upon the pressure at this time step.
        	m_medium->Create_refractive_map(currentPressure,
            	                            current_rhox,
            	                            current_rhoy,
            	                            current_rhoz,
            	                            rho0,
            	                            c2,
            	                            pezio_optical_coeff);	
		}
		/// Similar to above (i.e. sim_refractive_grad)
		else if (sim_displacement)
		{
        	/// Create a displacment map based upon the pressure at this time step.
        	m_medium->Create_displacement_map(currentVelocity_Xaxis,
            	                              currentVelocity_Yaxis,
            	                              currentVelocity_Zaxis,
            	                              m_medium->kwave.US_freq,
            	                              m_medium->kwave.dt);
        }
	

		/// Decide what to simulate (refractive gradient, displacement) based on whether those objects exist.
		m_medium->kwave.nmap == NULL ? da_boost->Simulate_refractive_gradient(false) :
                                       da_boost->Simulate_refractive_gradient(true);

		/// Decide what to simulate (refractive gradient, displacement) based on whether those objects exist.
        m_medium->kwave.dmap == NULL ? da_boost->Simulate_displacement(false) :
                                       da_boost->Simulate_displacement(true);
        
        /// Not saving seeds, so set to false.
        da_boost->Save_RNG_Seeds(false);
  
#undef DEBUG
#ifdef DEBUG
        /// Look at the middle of the medium, presumably where the focus is and the largest pressure and velocities.
        /// Assuming focal depth is 10 mm, we need to locate the correct voxel where this is located.
      
//        size_t x_voxel = 293;  /// Using inspected voxel from PA_guided focus.
//        size_t y_voxel = m_medium->Get_Ny()/2;
//        size_t z_voxel = m_medium->Get_Nz()/2;
//        float velX  = currentVelocity_Xaxis->GetElementFrom3D(x_voxel, y_voxel, z_voxel);
//        float velY  = currentVelocity_Yaxis->GetElementFrom3D(x_voxel, y_voxel, z_voxel);
//        float velZ  = currentVelocity_Zaxis->GetElementFrom3D(x_voxel, y_voxel, z_voxel);
//        float dispX = m_medium->kwave.dmap->getDisplacementFromGridX(x_voxel, y_voxel, z_voxel);
//        float dispY = m_medium->kwave.dmap->getDisplacementFromGridY(x_voxel, y_voxel, z_voxel);
//        float dispZ = m_medium->kwave.dmap->getDisplacementFromGridZ(x_voxel, y_voxel, z_voxel);
//                                                                    
//        Logger::getInstance()->Write_velocity_displacement(velX, velY, velZ,
//                                                           dispX, dispY, dispZ);
        
	       
#else
        
        /// Only run the MC-sim after ultrasound has propagated a certain distance (or time).
		/// Similar to the stroboscopic experiments.
        static size_t cnt = MC_time_step/Parameters->Get_dt();
        
        if (((KSpaceSolver->GetTimeIndex() % cnt) == 0) && 
			 (KSpaceSolver->GetTimeIndex() > 0))
        {
            cout << ".......... Running MC-Boost ......... ";
            cout << "(time: " << KSpaceSolver->GetTimeIndex()*Parameters->Get_dt() << ")\n";
            da_boost->Run_seeded_MC_sim_timestep(m_medium,
                                                 m_Laser_injection_coords,
                                                 KSpaceSolver->GetTimeIndex());

        }
#endif
        
        
        ///
        /// --------------------------- End Monte-Carlo Simulation ------------------------------------------------------
        //-- store the initial pressure at the first time step --//
        KSpaceSolver->FromMain_StoreSensorData();

        KSpaceSolver->FromMain_PrintStatistics();

    }


    KSpaceSolver->IterationTimeStop();

    cout << "-------------------------------------------------------------\n";           
    cout << "Elapsed time: " << KSpaceSolver->GetSimulationTime() << "\n";
    cout << "-------------------------------------------------------------\n";           
    
    cout << "Post-processing phase......."; cout.flush();       
    KSpaceSolver->PostProcessingTimeStart();
  

    KSpaceSolver->FromMain_PostProcessing();  
    KSpaceSolver->PostProcessingTimeStop();
    cout << "Done \n";
    cout << "Elapsed time: " << KSpaceSolver->GetPostProcessingTime() << "\n";

  
    KSpaceSolver->FromMain_WriteOutputDataInfo();
  
    Parameters->HDF5_OutputFile.Close();
  
    
}




void
AO_Sim::kWave_allocate_memory()
{
    assert(KSpaceSolver != NULL);
    try {
        KSpaceSolver->AllocateMemory();
    } catch (exception e){
        cout << "Failed!\nK-Wave panic: Not enough memory to run this simulation!\n";
        cout << e.what() << "\n";
        cerr << "K-Wave panic: Not enough memory to run this simulation!\n";
        cerr << e.what() << "\n";
        
        exit(EXIT_FAILURE);
    }
    
    cout << ".... done\n";

}


// Creates the grid for the monte-carlo simulation based upon the dimensions
// used in the k-Wave simulation.
//
void
AO_Sim::Create_MC_grid(TParameters * parameters)
{
    
    /// NOTE:  There is a transormation of z-axis (kWave) to x-axis (Monte-carlo)
    ///        so that ultrasound propagates orthogonally to light.  The object
    ///        'parameters' is defined from kWave, so we assign values from
    ///        'parameters' to the appropriate Monte-carlo coordinates to achieve
    ///        the rotation.
    ///--------------------------------------------------------------------------
    // Number of voxels in each axis, with the PML taken into account.
    // NOTE:
    // - It is assumed that the PML is always included INSIDE the computational
    //   medium (i.e. it is not expanded), as defined in the matlab script, and that there is a small
    //   cushion (5 voxels along US transmission axis) to ensure the probe sits away from the PML.
    //   These things are taken into account below in the monte-carlo simulation grid.
    // - As an example, if the x-axis is 512 elements and the X_PML is 20 elements,
    //   the US probe should sit at X_PML+OFFSET in the k-Wave computation medium, which
    //   means the monte-carlo medium should reflect these changes because US data in the PML
    //   is heavily distorted, as it should be.
    size_t OFFSET = 5; /// An offset to apply to the transmission axis PML (i.e. x_pml).
    size_t x_pml_offset = parameters->Get_pml_x_size()+OFFSET;
    size_t y_pml_offset = parameters->Get_pml_y_size();
    size_t z_pml_offset = parameters->Get_pml_z_size();
    
    size_t Nx = parameters->GetFullDimensionSizes().X - 2*x_pml_offset;
    size_t Ny = parameters->GetFullDimensionSizes().Y - 2*y_pml_offset;
    size_t Nz = parameters->GetFullDimensionSizes().Z - 2*z_pml_offset;
    
    // Size of each voxel along its respective axis.
    double dx = parameters->Get_dx();
    double dy = parameters->Get_dy();
    double dz = parameters->Get_dz();
    

    // Create the medium object that represents the grid for the
    // monte-carlo simulation based upon the dimensions for simulating
    // ultrasound.
    //
    m_medium = new Medium(Nx*dx,
                          Ny*dy,
                          Nz*dz);
    
    
    
    // Set the voxel sizes of the 'Medium'.
    this->m_medium->Set_dx(dx);
    this->m_medium->Set_dy(dy);
    this->m_medium->Set_dz(dz);
    
    // Set the number of voxels of the 'Medium'.
    this->m_medium->Set_Nx(Nx);
    this->m_medium->Set_Ny(Ny);
    this->m_medium->Set_Nz(Nz);
    
    /// Set the size of the PML's used in the k-Wave simulation.
    this->m_medium->Set_X_PML(x_pml_offset);
    this->m_medium->Set_Y_PML(y_pml_offset);
    this->m_medium->Set_Z_PML(z_pml_offset);
    
    
    /// Set the size of the sensor mask used in the kWave simulation.  This is the number
    /// of elements in the TRealMatrix that recorded data.
    // const size_t  sensor_size = Get_sensor_mask_ind().GetTotalElementCount();
    this->m_medium->kwave.sensor_mask_index_size    = parameters->Get_sensor_mask_index_size();
    
    /// Set the time-step used in k-Wave.
    this->m_medium->kwave.dt = parameters->Get_dt();
    //this->m_medium->kwave.speed_of_sound            = parameters->Get_c0_scalar();
    
    
}




