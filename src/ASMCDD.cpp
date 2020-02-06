//
// Created by "Dylan Brasseur" on 28/11/2019.
//

#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <cassert>
#include <queue>
#include "../include/ASMCDD.h"
#include "../include/Category.h"
#include "../include/computeFunctions.h"

unsigned long ASMCDD::addTargetClass(std::vector<Disk> const &target){
    categories->emplace_back(categories->size(), categories, params);
    categories->back().setTargetDisks(target);
    return categories->size()-1;
}

Category &ASMCDD::getClass(unsigned long id){
    return categories->at(id);
}

void ASMCDD::setParams(ASMCDD_params const &_params){
    (*params.get()) = _params;
}

void ASMCDD::addDependency(unsigned long parent, unsigned long child){
    categories->at(parent).addChild(child);
    categories->at(child).addDependency(parent);
}

void ASMCDD::computeTarget(){
    for(auto & category : (*categories.get()))
    {
        category.computeTarget();
    }
}

std::vector<Target_pcf_type> ASMCDD::getTargetPCF(unsigned long parent, unsigned long child){
    return categories->at(child).getTargetPCF(parent);
}

std::vector<Target_pcf_type> ASMCDD::getCurrentPCF(unsigned long parent, unsigned long child){
    return categories->at(child).getCurrentPCF(parent);
}

std::vector<Target_pcf_type> ASMCDD::getTargetPCF(unsigned long self){
    return getTargetPCF(self, self);
}

std::vector<Target_pcf_type> ASMCDD::getCurrentPCF(unsigned long self){
    return getCurrentPCF(self, self);
}

void ASMCDD::initialize(float domainLength, float e_delta){
    for(auto & category : (*categories.get()))
    {
        category.initialize(domainLength, e_delta);
    }
}

std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> ASMCDD::getCurrentPCFplot(){
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> result;
    for(auto & category : (*categories.get()))
    {
        auto value = category.getCurrentPCFs();
        result.reserve(result.size()+value.size());
        result.insert(result.end(), value.begin(), value.end());
    }
    return result;
}

std::vector<Disk> ASMCDD::getCurrentDisks(unsigned long id){
    return categories->at(id).getCurrentDisks();
}

void ASMCDD::loadFile(std::string const & filename){
    int buffer;
    std::ifstream file;
    file.open(filename);
    assert(file.good());
    if(!(file >> buffer))
    {
        std::cout << "ERREUR" << std::endl;
        exit(1);
    }
    unsigned int n_classes = buffer;
    std::map<unsigned int, unsigned int> id_map;
    auto cats = categories.get();
    cats->clear();
    for(unsigned int i=0; i<n_classes; i++)
    {
        file >> buffer;
        id_map.insert_or_assign(buffer, i);
        cats->emplace_back(i, categories, params);
    }
    while(file >> buffer)
    {
        float x, y, r;
        unsigned int index = id_map[buffer];
        file >> x >> y >>r;
        x/=10000.f;
        y/=10000.f;
        r/=10000.f;
        (*cats)[index].addTargetDisk({x,y,r});
    }
    file.close();
}

ASMCDD::ASMCDD(std::string const &filename) : ASMCDD::ASMCDD(){
    loadFile(filename);
}

std::vector<Disk> ASMCDD::getTargetDisks(unsigned long id){
    return categories->at(id).getTargetDisks();
}

std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> ASMCDD::getTargetPCFplot(){
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> result;
    for(auto & category : (*categories.get()))
    {
        auto value = category.getTargetPCFs();
        result.reserve(result.size()+value.size());
        result.insert(result.end(), value.begin(), value.end());
    }
    return result;
}

void ASMCDD::normalize(float domainLength){
    for(auto & c : *categories.get())
    {
        c.normalize(domainLength);
    }
}

std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> ASMCDD::getPrettyPCFplot(float domainLength, std::vector<unsigned long> const &currentSizes){
    std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> plots;
    float diskfactor = 1/domainLength;
    //Get all the disks and info
    std::vector<Compute_status> compute_stats;
    for(auto & c : *categories.get())
    {
        compute_stats.push_back(c.getComputeStatus());
    }
    //Get the pretty pcfs from them

    //Compute common weight terms
    auto nSteps = (unsigned long)(params->limit/params->step);
    std::vector<std::vector<float>> radii;
    std::vector<std::vector<float>> area;
    radii.resize(compute_stats.size());
    area.resize(compute_stats.size());
    float totalRmax;
    std::vector<float> totalRadii;
    std::vector<float> totalArea;
    for(unsigned long c=0; c<compute_stats.size(); c++)
    {
        float rmax = compute_stats[c].rmax;
        radii[c].resize(nSteps);
        area[c].resize(nSteps);
        for(unsigned long k=0; k<nSteps; k++)
        {
            radii[c][k] = (k+1)*params->step*rmax;
            float inner = std::max(0.f, radii[c][k]-0.5f*rmax);
            float outer = radii[c][k]+0.5f*rmax;
            area[c][k] = M_PI*(outer*outer - inner*inner);
        }
    }
    auto finalSizes = getFinalSizes(1);
    totalRmax = computeRmax(std::accumulate(finalSizes.begin(), finalSizes.end(), 0UL));
    totalRadii.resize(nSteps);
    totalArea.resize(nSteps);
    for(unsigned long k=0; k<nSteps; k++)
    {
        totalRadii[k] = (k+1)*params->step*totalRmax;
        float inner = std::max(0.f, totalRadii[k]-0.5f*totalRmax);
        float outer = totalRadii[k]+0.5f*totalRmax;
        totalArea[k] = M_PI*(outer*outer - inner*inner);
    }

    for(unsigned long c=0; c<compute_stats.size(); c++)
    {
        auto & stat = compute_stats[c];
        std::vector<std::pair<float, float>> plot;
        plots.first.push_back(stat.disks);
        if(currentSizes[c] != stat.disks.size())
        {
            std::vector<float> pcf = compute_pretty_pcf(stat.disks, stat.disks, radii[c], area[c], stat.rmax, *params.get(), diskfactor);
            plot.resize(pcf.size());
            for(unsigned long k=0; k<radii[c].size(); k++)
            {
                plot[k].first = radii[c][k]/(stat.rmax);
                plot[k].second = pcf[k]*domainLength*domainLength;
            }
            plots.second.emplace_back(std::make_pair(c, c), plot);
        }
        for(unsigned long other : stat.parents)
        {
            if(currentSizes[c] != stat.disks.size() || currentSizes[other] != compute_stats[other].disks.size())
            {
                std::vector<float> pcf = compute_pretty_pcf(stat.disks, compute_stats[other].disks, totalRadii, totalArea, totalRmax, *params.get(), diskfactor);
                plot.resize(pcf.size());
                for(unsigned long k=0; k<radii[c].size(); k++)
                {
                    plot[k].second = pcf[k]*domainLength*domainLength;
                }
                plots.second.emplace_back(std::make_pair(other, c), plot);
            }
        }
    }

    return plots;
}

std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> ASMCDD::getPrettyTargetPCFplot(float domainLength){
    std::pair<std::vector<std::vector<Disk>>, std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>>> plots;
    float diskfactor = 1 / domainLength;
    //Get all the disks and info
    std::vector<Compute_status> compute_stats;
    for(auto &c : *categories.get()){
        compute_stats.push_back(c.getTargetComputeStatus());
    }
    //Get the pretty pcfs from them

    //Compute common weight terms
    auto nSteps = (unsigned long)(params->limit / params->step);
    std::vector<std::vector<float>> radii;
    std::vector<std::vector<float>> area;
    radii.resize(compute_stats.size());
    area.resize(compute_stats.size());
    float totalRmax;
    std::vector<float> totalRadii;
    std::vector<float> totalArea;
    unsigned long nbPoints = 0;
    for(unsigned long c = 0; c < compute_stats.size(); c++){
        nbPoints += compute_stats[c].disks.size();
        float rmax = compute_stats[c].rmax;
        radii[c].resize(nSteps);
        area[c].resize(nSteps);
        for(unsigned long k = 0; k < nSteps; k++){
            radii[c][k] = (k + 1) * params->step * rmax;
            float inner = std::max(0.f, radii[c][k] - 0.5f * rmax);
            float outer = radii[c][k] + 0.5f * rmax;
            area[c][k] = M_PI * (outer * outer - inner * inner);
        }
    }

    totalRmax = computeRmax(nbPoints);
    totalRadii.resize(nSteps);
    totalArea.resize(nSteps);
    for(unsigned long k = 0; k < nSteps; k++){
        totalRadii[k] = (k + 1) * params->step * totalRmax;
        float inner = std::max(0.f, totalRadii[k] - 0.5f * totalRmax);
        float outer = totalRadii[k] + 0.5f * totalRmax;
        totalArea[k] = M_PI * (outer * outer - inner * inner);
    }

    for(unsigned long c = 0; c < compute_stats.size(); c++){
        auto &stat = compute_stats[c];
        plots.first.push_back(stat.disks);
        std::vector<float> pcf = compute_pretty_pcf(stat.disks, stat.disks, radii[c], area[c], stat.rmax, *params.get(),
                                                    diskfactor);
        std::vector<std::pair<float, float>> plot;
        plot.resize(pcf.size());
        for(unsigned long k = 0; k < radii[c].size(); k++){
            plot[k].first = radii[c][k] / stat.rmax;
            plot[k].second = pcf[k];
        }
        plots.second.emplace_back(std::make_pair(c, c), plot);
        for(unsigned long other : stat.parents){
            pcf = compute_pretty_pcf(stat.disks, compute_stats[other].disks, totalRadii, totalArea, totalRmax, *params.get(),
                                     diskfactor);
            for(unsigned long k = 0; k < radii[c].size(); k++){
                plot[k].second = pcf[k];
            }
            plots.second.emplace_back(std::make_pair(other, c), plot);
        }
    }

    return plots;
}

std::vector<unsigned long> ASMCDD::getFinalSizes(float domainLength){
    std::vector<unsigned long> sizes;
    for(auto & c : *categories.get())
    {
        sizes.push_back(c.getFinalSize(domainLength));
    }
    return sizes;
}

void ASMCDD::refine(unsigned long max_iter, float threshold, bool isDistanceThreshold){
    //TODO
}
