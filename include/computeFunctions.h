//
// Created by "Dylan Brasseur" on 06/02/2020.
//

#ifndef DISKSPROJECT_COMPUTEFUNCTIONS_H
#define DISKSPROJECT_COMPUTEFUNCTIONS_H

#include <vector>
#include "utils.h"

std::vector<float> compute_density(Disk const & pi, std::vector<Disk> const & others, std::vector<float> const & areas, std::vector<float> const & radii, float rmax, ASMCDD_params const & params, unsigned long same_category_index, unsigned long target_size);
std::vector<float> get_weight(Disk const & d, std::vector<float> const & radii, float diskfactor);
std::vector<std::vector<float>> get_weights(std::vector<Disk> const & disks, std::vector<float> const & radii, float diskfactor);
Contribution compute_contribution(Disk const & pi, std::vector<Disk> const & others, std::vector<std::vector<float>> other_weights, std::vector<float> const & radii, std::vector<float> const & areas, float rmax, ASMCDD_params const & params, unsigned long same_category_index, unsigned long target_size, float diskfactor);
std::vector<Target_pcf_type> compute_pcf(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, std::vector<float> const & area, std::vector<float> const & radii, float rmax, ASMCDD_params const & params);
float compute_error(Contribution const & contribution, std::vector<float> const & currentPCF, std::vector<Target_pcf_type> const & target);
std::vector<float> compute_pretty_pcf(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, std::vector<float> const & radii, std::vector<float> const & area, float rmax, ASMCDD_params const & params, float diskfactor);
#endif //DISKSPROJECT_COMPUTEFUNCTIONS_H
