/*
 * displacementMap.cpp
 *
 *  Created on: 3 aug. 2011
 *      Author: StaleyJW
 */

#include "debug.h"
#include <MC-Boost/vector3D.h>
#include "displacementMap.h"
#include <boost/lexical_cast.hpp>


// It's an error to create a DisplacementMap object without specifying attributes,
// therefore the default constructor should never be called.
DisplacementMap::DisplacementMap()
: X_PML_OFFSET(25), // defaults
  Y_PML_OFFSET(10),
  Z_PML_OFFSET(10)
{
    displacement_map_x = displacement_map_y = displacement_map_z = NULL;
}



DisplacementMap::~DisplacementMap()
{
//	if (displacement_gridX)
//		delete displacement_gridX;
//
//	if (displacement_gridY)
//		delete displacement_gridY;
//
//	if (displacement_gridZ)
//		delete displacement_gridZ;
//    
//    
//    
//    if (displacement_map_x)
//    {
//        delete displacement_map_x;
//        displacement_map_x = NULL;
//    }
//    
//    if (displacement_map_y)
//    {
//        delete displacement_map_y;
//        displacement_map_y = NULL;
//    }
//    
//    if (displacement_map_z)
//    {
//        delete displacement_map_z;
//        displacement_map_z = NULL;
//    }
//    
}

 
// Returns the individual axis displacement value from their location in the grid.
double DisplacementMap::getDisplacementFromGridX(const int x_photon, const int y_photon, const int z_photon)
{
	//return (*displacement_gridX)[(array_index)a][(array_index)b][(array_index)c];
    
    return Get_displacement_X_TRealMatrix(x_photon,
                                          y_photon,
                                          z_photon);
}

double DisplacementMap::getDisplacementFromGridY(const int x_photon, const int y_photon, const int z_photon)
{
	//return (*displacement_gridY)[(array_index)a][(array_index)b][(array_index)c];
    
    return Get_displacement_Y_TRealMatrix(x_photon,
                                          y_photon,
                                          z_photon);
}

double DisplacementMap::getDisplacementFromGridZ(const int x_photon, const int y_photon, const int z_photon)
{
	//return (*displacement_gridZ)[(array_index)a][(array_index)b][(array_index)c];
    
    return Get_displacement_Z_TRealMatrix(x_photon,
                                          y_photon,
                                          z_photon);
}



/// Invert the phase of the displacement data 180 degrees by multiplying through the matrix by -1.
void DisplacementMap::Invert_phase(void)
{
    float * raw_data_x = displacement_map_x->GetRawData();
    float * raw_data_y = displacement_map_y->GetRawData();
    float * raw_data_z = displacement_map_z->GetRawData();

    /// NOTE:
    /// - The total element count is the same for each axial component of the displacement.
    const size_t sensor_size  = displacement_map_x->GetTotalElementCount();

    for (size_t i = 0; i < sensor_size; i++)
    {
        /// Perform the inversion.
        raw_data_x[i] *= -1.0f;
        raw_data_y[i] *= -1.0f;
        raw_data_z[i] *= -1.0f;
    }
}





