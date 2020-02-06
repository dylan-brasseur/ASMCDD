//
// Created by "Dylan Brasseur" on 06/02/2020.
//
#include <algorithm>
#include <random>
#include "../include/Category.h"
#include "../include/computeFunctions.h"

std::mutex Category::disks_access;

void Category::setTargetDisks(std::vector<Disk> const &target){
    target_disks = target;
}

void Category::addDependency(unsigned long parent_id){
    if(std::find(parents_id.begin(), parents_id.end(), parent_id) == parents_id.end())
    {
        parents_id.push_back(parent_id);
    }
}

void Category::addChild(unsigned long child_id){
    if(std::find(children_id.begin(), children_id.end(), child_id) == children_id.end())
    {
        children_id.push_back(child_id);
    }
}

void Category::computeTarget(){
    target_pcf.clear();
    target_rmax.clear();
    target_radii.clear();

    if(target_disks.empty())
    {
        return;
    }

    auto nSteps = (unsigned long)(params->limit/params->step);

    target_rmax.insert(std::make_pair(id, computeRmax(target_disks.size())));
    for(unsigned long parent : parents_id)
    {
        target_rmax.insert(std::make_pair(parent, computeRmax(target_disks.size()/*+(*categories.get())[parent].target_disks.size()*/)));
    }
    std::vector<float> area, radii;
    area.resize(nSteps);
    radii.resize(nSteps);
    std::vector<unsigned long> relations;
    relations.push_back(id);
    relations.insert(relations.end(),parents_id.begin(), parents_id.end());
    for(unsigned long parent : relations)
    {
        float rmax = target_rmax[parent];
        for(unsigned long i=0; i<nSteps; i++)
        {
            float r = (i+1)*params->step;
            float outer = (r+0.5f)*rmax;
            float inner = std::max((r-0.5f)*rmax, 0.f);
            area[i] = M_PI*(outer*outer - inner*inner);
            radii[i] = r*rmax;
        }
        target_areas.insert(std::make_pair(parent, area));
        target_radii.insert(std::make_pair(parent, radii));

        auto & parent_disks = (*categories.get())[parent].target_disks;
        target_pcf.insert(std::make_pair(parent, compute_pcf(target_disks, parent_disks, area, radii, rmax, *params.get())));
    }
}



void Category::initialize(float domainLength, float e_delta){
    if(initialized)
        return;
    std::random_device rand_device;
    std::mt19937_64 rand_gen(rand_device());

    disks.clear();
    pcf.clear();

    for(unsigned long parent : parents_id)
    {
        (*categories.get())[parent].initialize(domainLength, e_delta);
    }

    std::vector<float> output_disks_radii;

    float n_factor = domainLength*domainLength;
    float diskfact = 1/domainLength;
    unsigned long long n_repeat = std::ceil(n_factor);
    output_disks_radii.reserve(n_repeat*target_disks.size());
    for(auto & d : target_disks)
    {
        for(unsigned long long i=0; i<n_repeat; i++)
        {
            output_disks_radii.push_back(d.r);
        }
    }
    std::uniform_real_distribution<float> randf(0, domainLength);

    std::shuffle(output_disks_radii.begin(), output_disks_radii.end(), rand_gen); //Shuffle array
    output_disks_radii.resize(target_disks.size()*n_factor); // and resize it to the number of disks we want
    // This combination effectively does a random non repeating sampling

    std::sort(output_disks_radii.rbegin(), output_disks_radii.rend()); //Sort the radii in descending order
    finalSize = output_disks_radii.size();

    float e_0 = 0;
    unsigned long max_fails=1000;
    unsigned long fails=0;
    unsigned long n_accepted=0;
    disks.reserve(output_disks_radii.size());
    auto nSteps = (unsigned long)(params->limit/params->step);
    std::vector<unsigned long> relations;
    relations.reserve(1+parents_id.size());
    relations.push_back(id);
    relations.insert(relations.end(), parents_id.begin(), parents_id.end());
    auto & others = *categories.get();
    auto & parameters = *params.get();

    constexpr unsigned long MAX_LONG = std::numeric_limits<unsigned long>::max();
    std::map<unsigned long, std::vector<std::vector<float>>> weights;
    std::map<unsigned long, std::vector<float>> current_pcf;
    for(auto relation : relations){
        current_pcf.insert(std::make_pair(relation, 0));
        current_pcf[relation].resize(nSteps, 0);
        weights.insert(std::make_pair(relation, get_weights(others[relation].disks, target_radii[relation], diskfact)));
    }

    std::map<unsigned long, Contribution> contributions;

    do{
        bool rejected=false;
        float e = e_0 + e_delta*fails;
        Disk d_test(randf(rand_gen), randf(rand_gen), output_disks_radii[n_accepted]);
        for(auto relation : relations)
        {
            Contribution test_pcf;
            if(!disks.empty() || relation != id)
            {
                test_pcf = compute_contribution(d_test, others[relation].disks, weights[relation], target_radii[relation], target_areas[relation], target_rmax[relation], parameters, relation == id ? n_accepted : MAX_LONG,relation == id ? 2*output_disks_radii.size()*output_disks_radii.size() : 2*output_disks_radii.size()*others[relation].disks.size(), diskfact);
                if(e < compute_error(test_pcf, current_pcf[relation], target_pcf[relation]))
                {
                    rejected=true;
                    break;
                }
            }else{
                test_pcf.pcf.resize(nSteps, 0);
                test_pcf.contribution.resize(nSteps, 0);
                test_pcf.weights = get_weight(d_test, target_radii[relation], diskfact);
            }
            contributions.insert_or_assign(relation, test_pcf);
        }
        if(rejected)
        {
            fails++;
        }else
        {
            disks_access.lock();
            disks.push_back(d_test);
            disks_access.unlock();
            fails=0;
            for(auto relation : relations)
            {
                auto & current = current_pcf[relation];
                auto & contrib = contributions[relation];
                if(relation == id)
                {
                    weights[relation].emplace_back(contrib.weights);
                }
                for(unsigned long k=0; k<nSteps; k++)
                {
                    current[k]+=contrib.contribution[k];
                }
            }
            n_accepted++;
        }

        if(fails > max_fails)
        {
            std::cout << "Grid searching : " << id <<std::endl;
            //Grid search
            constexpr unsigned long N_I = 100;
            constexpr unsigned long N_J = 100;
            std::map<unsigned long, Contribution> contribs[N_I][N_J];
            while(n_accepted < output_disks_radii.size())
            {
                float errors[N_I+1][N_J+1];
                Compare minError = {INFINITY,0, 0};
#pragma omp parallel for default(none) collapse(2) shared(output_disks_radii, n_accepted, relations, others, parameters, nSteps, errors, diskfact, contribs, weights, current_pcf, domainLength)
                for(unsigned long i=1; i<N_I; i++)
                {
                    for(unsigned long j=1; j<N_J; j++)
                    {
                        float currentError=0;
                        Disk cell_test((domainLength/N_I)*i, (domainLength/N_J)*j, output_disks_radii[n_accepted]);
                        for(auto && relation : relations)
                        {
                            Contribution test_pcf;
                            test_pcf = compute_contribution(cell_test, others[relation].disks, weights[relation], target_radii[relation], target_areas[relation], target_rmax[relation], parameters, relation == id ? n_accepted : MAX_LONG, relation == id ? output_disks_radii.size()*output_disks_radii.size() : output_disks_radii.size()*others[relation].disks.size(), diskfact);
                            currentError = std::max(currentError, compute_error(test_pcf, current_pcf[relation], target_pcf[relation]));
                            contribs[i][j].insert_or_assign(relation, test_pcf);
                        }

                        errors[i][j] = currentError;
                    }
                }

                for(unsigned long i=1; i<N_I; i++)
                {
                    for(unsigned long j=1; j<N_J; j++)
                    {
                        if(errors[i][j] < minError.val)
                        {
                            minError.val = errors[i][j];
                            minError.i = i;
                            minError.j = j;
                        }
                    }
                }

                disks_access.lock();
                disks.emplace_back((domainLength/N_I)*minError.i + (randf(rand_gen)-domainLength/2)/(N_I*10), (domainLength/N_J)*minError.j + (randf(rand_gen)-domainLength/2)/(N_J*10), output_disks_radii[n_accepted]);
                disks_access.unlock();
                for(auto relation : relations)
                {
                    auto & current = current_pcf[relation];
                    auto & contrib = contribs[minError.i][minError.j][relation];
                    if(relation == id)
                    {
                        weights[relation].emplace_back(contrib.weights);
                    }
                    for(unsigned long k=0; k<nSteps; k++)
                    {
                        current[k]+=contrib.contribution[k];
                    }
                }
                n_accepted++;
            }

        }
    }while(n_accepted < output_disks_radii.size());
    for(auto r : relations)
    {
        std::vector<Target_pcf_type> cpcf;
        cpcf.resize(nSteps, {0,0,0});
        for(unsigned long k=0; k<nSteps; k++)
        {
            cpcf[k].mean = current_pcf[r][k];
            cpcf[k].radius = (k+1)*parameters.step;
        }
        //pcf.insert_or_assign(r, cpcf);
        pcf.insert_or_assign(r, compute_pcf(disks, others[r].disks, target_areas[r], target_radii[r], target_rmax[r], parameters));
    }
    initialized=true;

}

std::vector<Target_pcf_type> Category::getCurrentPCF(unsigned long parent){
    return pcf.at(parent);
}

std::vector<Target_pcf_type> Category::getTargetPCF(unsigned long parent){
    return target_pcf.at(parent);
}

std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> Category::getCurrentPCFs(){
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> result;
    result.reserve(pcf.size());
    for(auto & currpcf : pcf)
    {
        result.emplace_back(std::make_pair(currpcf.first, id), 0);
        auto & coords = result.back().second;
        coords.reserve(currpcf.second.size());
        for(auto & value : currpcf.second)
        {
            coords.emplace_back(value.radius, value.mean);
        }
    }

    return result;
}

std::vector<Disk> Category::getCurrentDisks(){
    disks_access.lock();
    std::vector<Disk> outDisks(disks);
    disks_access.unlock();
    return outDisks;
}

void Category::addTargetDisk(Disk const &d){
    target_disks.push_back(d);
}

std::vector<Disk> Category::getTargetDisks(){
    return target_disks;
}

std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> Category::getTargetPCFs(){
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> result;
    result.reserve(target_pcf.size());
    for(auto & currpcf : target_pcf)
    {
        result.emplace_back(std::make_pair(currpcf.first, id), 0);
        auto & coords = result.back().second;
        coords.reserve(currpcf.second.size());
        for(auto & value : currpcf.second)
        {
            coords.emplace_back(value.radius, value.mean);
        }
    }

    return result;
}

Compute_status Category::getComputeStatus()
{
    Compute_status status;
    status.rmax = target_rmax[id];
    status.disks = getCurrentDisks();
    status.parents = parents_id;
    return status;
}

Compute_status Category::getTargetComputeStatus()
{
    Compute_status status;
    status.rmax = target_rmax[id];
    status.disks = getTargetDisks();
    status.parents = parents_id;
    return status;
}

unsigned long Category::getFinalSize(float domainLength){
    return target_disks.size()*domainLength*domainLength;
}

void Category::normalize(float domainLength)
{
    for(auto & d : disks)
    {
        d.x/=domainLength;
        d.y/=domainLength;
        d.r/=domainLength;
    }
}

void Category::refine(unsigned long max_iter, float threshold, bool isDistanceThreshold){
    //TODO
}