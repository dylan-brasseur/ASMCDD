//
// Created by "Dylan Brasseur" on 28/11/2019.
//

#ifndef DISKSPROJECT_ASMCDD_H
#define DISKSPROJECT_ASMCDD_H

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <random>
#include <mutex>
#include "utils.h"
#include "Category.h"

/**
 * This class is the backbone of the algorithm
 */
class ASMCDD{
public:
    explicit ASMCDD(){ categories=std::make_shared<std::vector<Category>>(); params=std::make_shared<ASMCDD_params>();};
    explicit ASMCDD(std::string const & filename);

    /**
     * Loads an example file
     * \param filename Path to the file
     */
    void loadFile(std::string const & filename);
    /**
     * \deprecated Load an example file instead
     * Manually adds a class
     * \param target Disks of the class
     * \return class id
     */
    unsigned long addTargetClass(std::vector<Disk> const & target);

    Category & getClass(unsigned long id);

    void setParams(ASMCDD_params const & _params);
    /**
     * Computes the pcf of the target disks
     */
    void computeTarget();

    /**
     * Adds a directed edge in the dependency graph of the classes
     * \param parent Parent class id
     * \param child Child class id
     */
    void addDependency(unsigned long parent, unsigned long child);

    /**
     * Initialization part of the algorithm
     * \param domainLength Length of the square domain
     * \param e_delta Error delta to add at each failed dart throw
     */
    void initialize(float domainLength, float e_delta);

    /**
     * Refinement part of the algorithm
     * CURRENTLY NOT IMPLEMENTED
     * \param max_iter Max iterations of the refinement
     * \param threshold Threshold
     * \param isDistanceThreshold true if the threshold is distance based, otherwise it's pcf error based
     */
    void refine(unsigned long max_iter, float threshold, bool isDistanceThreshold);

    /**
     * Normalizes the disks to a length 1 domain
     * \warning Modifies the values inside the classes, use at your own risk
     * \param domainLength Length of the domain
     */
    void normalize(float domainLength);

    std::vector<Target_pcf_type> getTargetPCF(unsigned long self);
    std::vector<Target_pcf_type> getTargetPCF(unsigned long parent, unsigned long child);
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long self);
    std::vector<Target_pcf_type> getCurrentPCF(unsigned long parent, unsigned long child);

    /**
     * Functions to get the pcf plots unsed in computation
     */
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getTargetPCFplot();
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> getCurrentPCFplot();

    /**
     * Functions to get the current disks
     * \param id Id of the class
     */
    std::vector<Disk> getCurrentDisks(unsigned long id);
    /**
     * Functions to get the target disks
     * \param id Id of the class
     */
    std::vector<Disk> getTargetDisks(unsigned long id);

    /**
     * Gets the current disks and pcf plots in a "pretty" form, not following the pcf used in computation
     * \param domainLength Length of the domain
     * \param currentSizes Array in which the current sizes of the disk arrays will be written
     * \return
     */
    std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> getPrettyPCFplot(float domainLength, std::vector<unsigned long> const & currentSizes);

    /**
     * gets the target disks and pcf plots in a "pretty" form, not following the pcf used in computation
     * \param domainLength length of the target domain (SHOULD BE 1)
     * \return
     */
    std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> getPrettyTargetPCFplot(float domainLength);

    /**
     * Returns the final number of disks for each class for a domain length
     * \param domainLength Domain Length
     * \return
     */
    std::vector<unsigned long> getFinalSizes(float domainLength);

private:
    std::shared_ptr<std::vector<Category>> categories;
    std::shared_ptr<ASMCDD_params> params;
};

#endif //DISKSPROJECT_ASMCDD_H
