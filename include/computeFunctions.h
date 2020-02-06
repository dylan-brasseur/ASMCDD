//
// Created by "Dylan Brasseur" on 06/02/2020.
//

#ifndef DISKSPROJECT_COMPUTEFUNCTIONS_H
#define DISKSPROJECT_COMPUTEFUNCTIONS_H

#include <vector>
#include "utils.h"

/*
 * These functions are at the heart of the algorithm and provide the heavy duty computation
 */
/**
 * Gets the individual pcf of the disk pi
 * \param pi Disk of interest
 * \param others Other disks to compute the pcf
 * \param areas Areas of the used radii rings
 * \param radii Used radii
 * \param rmax Rmax for the given pcf
 * \param params Parameters of the program
 * \param same_category_index Index of pi if the pcf is being computed on the same array
 * \param target_size Size of the end array of disks
 * \return individual PCF of pi
 */
std::vector<float> compute_density(Disk const & pi, std::vector<Disk> const & others, std::vector<float> const & areas, std::vector<float> const & radii, float rmax, ASMCDD_params const & params, unsigned long same_category_index, unsigned long target_size);

/**
 * Gets the weights for the given disk
 * \param d Disk of interest
 * \param radii Radii to test
 * \param diskfactor Disk size factor depending on domain length
 * \return
 */
std::vector<float> get_weight(Disk const & d, std::vector<float> const & radii, float diskfactor);

/**
 * Gets the weights for the disks
 * Calls get_weight
 * \return
 */
std::vector<std::vector<float>> get_weights(std::vector<Disk> const & disks, std::vector<float> const & radii, float diskfactor);

/**
 * Computes the partial contribution of the disk to the PCF
 * \param pi Disk of interest
 * \param others Other disks
 * \param other_weights Weights of the other disks
 * \param radii Radii to use
 * \param areas Area for the disks to use
 * \param rmax rmax for the given pcf
 * \param params Algorithm parameters
 * \param same_category_index Index of the disk if it's in the same array as tested
 * \param target_size End size of the disk array
 * \param diskfactor Disk size factor
 * \return
 */
Contribution compute_contribution(Disk const & pi, std::vector<Disk> const & others, std::vector<std::vector<float>> other_weights, std::vector<float> const & radii, std::vector<float> const & areas, float rmax, ASMCDD_params const & params, unsigned long same_category_index, unsigned long target_size, float diskfactor);

/**
 * Computes the pcf between 2 disk arrays (can be the same)
 * \param disks_a Disk array a
 * \param disks_b Disk array b
 * \param area Area for the radii to use
 * \param radii Radii to use
 * \param rmax Rmax for the pcf
 * \param params Algorithm parameters
 * \return
 */
std::vector<Target_pcf_type> compute_pcf(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, std::vector<float> const & area, std::vector<float> const & radii, float rmax, ASMCDD_params const & params);

/**
 * Computes the error between the contribution, the current pcf and the target pcf
 * \param contribution Disk individual contribution
 * \param currentPCF Current pcf of the disk category
 * \param target Target pcf
 * \return
 */
float compute_error(Contribution const & contribution, std::vector<float> const & currentPCF, std::vector<Target_pcf_type> const & target);

/**
 * Computes the pretty pcf between 2 sets of disks (not following the paper, for visual)
 * \param disks_a Disks a
 * \param disks_b Disks b
 * \param radii Radii to use
 * \param area Areas for the radii to use
 * \param rmax Rmax for the pcf
 * \param params Algorithm parameters
 * \param diskfactor Disk size factor
 * \return
 */
std::vector<float> compute_pretty_pcf(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, std::vector<float> const & radii, std::vector<float> const & area, float rmax, ASMCDD_params const & params, float diskfactor);
#endif //DISKSPROJECT_COMPUTEFUNCTIONS_H
