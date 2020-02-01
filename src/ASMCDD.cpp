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
#include <chrono>
#include "../include/ASMCDD.h"

std::mutex Category::disks_access;

template<typename T>
constexpr inline T max(T a, T b)
{
    return a > b ? a : b;
}

template<typename T>
constexpr inline T min(T a, T b)
{
    return a < b ? a : b;
}

inline float euclidian(Disk const & a, Disk const & b)
{
    return std::sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}
static float sqrtpi = std::sqrt(M_PI);
inline float gaussian_kernel(float sigma, float x)
{
    return std::exp(-((x*x)/(sigma*sigma)))/(sqrtpi*sigma);
}

//Returns the proportion of the circle perimeter that is in the [0, 1] domain
float perimeter_weight(double x, double y, double r)
{
    //We assume the domain is [0, 1] on all sides
    //Assuming the center x, y is in [0,1]

    double full_angle = 2*M_PI;
    const std::pair<double, double> deltas[] = {{x, y}, {1-x, y}, {y, x}, {1-y, x}};
    for(const auto & delta : deltas)
    {
        double dx = delta.first;
        if(dx < r)
        {
            double dy = delta.second;
            double alpha =std::acos(dx/r);
            full_angle-= std::min(alpha, std::atan2(dy, dx)) + std::min(alpha, std::atan2((1-dy), dx));
        }
    }
    return clip(full_angle/(2*M_PI), 0.0, 1.0);
}

float perimeter_weight(float x, float y, float r, float diskfact)
{
    return perimeter_weight(x*diskfact, y*diskfact, r*diskfact);
}

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

inline float computeRmax(unsigned long n)
{
    return float(2.0*std::sqrt(1/(2*std::sqrt(3.0)*double(n))));
}

float diskDistance(Disk const & a, Disk const & b, float rmax)
{
    //return euclidian(a,b)/rmax;
    float r1, r2;
    if(a.r > b.r)
    {
        r1 = a.r;
        r2 = b.r;
    }else
    {
        r1 = b.r;
        r2 = a.r;
    }
    r1/=rmax;
    r2/=rmax;
    float d = euclidian(a,b)/rmax;
    float extent = std::max(d+r1+r2, 2*r1);
    float overlap = clip(r1+r2-d, 0.0f, 2*r2);
    float f = (extent-overlap+d+r1-r2);
    if(d <= r1-r2)
    {
        return f/(4*r1 - 4*r2);
    }else if(d <= r1+r2)
    {
        return (f - 4*r1 + 7*r2)/(3*r2);
    }else{
        return f - 4*r1 - 2*r2 + 3;
    }
}


std::vector<float> compute_density(Disk const & pi, std::vector<Disk> const & others, std::vector<float> const & areas, std::vector<float> const & radii, float rmax, ASMCDD_params const & params, unsigned long same_category_index, unsigned long target_size)
{
    auto nSteps = (unsigned long)(params.limit/params.step);
    std::vector<float> weights, density;
    weights.resize(nSteps);
    density.resize(nSteps, 0);
    if(others.empty())
        return density;
    for(unsigned long k=0; k<nSteps; k++)
    {
        float perimeter = perimeter_weight(pi.x, pi.y, radii[k]);
        weights[k] = perimeter <= 0 ? 0.0f : 1.f/perimeter;
    }
    for(unsigned long j=0; j<others.size(); j++)
    {
        if(j == same_category_index)
            continue;
        auto & pj = others[j];
        float d = diskDistance(pi, pj, rmax);
        for(unsigned long k=0; k<nSteps; k++)
        {
            float r = radii[k]/rmax;
            density[k]+=gaussian_kernel(params.sigma, r-d);
        }
    }
    for(unsigned long k=0; k<nSteps; k++)
    {
        //d/=others.size();
        density[k]*=weights[k]/areas[k];
    }
    return density;
}

struct Contribution{
    std::vector<float> weights;
    std::vector<float> contribution;
    std::vector<float> pcf;
};

std::vector<float> get_weight(Disk const & d, std::vector<float> const & radii, float diskfactor)
{
    std::vector<float> weight;
    weight.resize(radii.size());
    for(unsigned long k=0; k<radii.size(); k++)
    {
        float perimeter = perimeter_weight(d.x, d.y, radii[k], diskfactor);
        weight[k] = perimeter <= 0 ? 0.0f : 1.f/perimeter;
    }
    return weight;
}

std::vector<std::vector<float>> get_weights(std::vector<Disk> const & disks, std::vector<float> const & radii, float diskfactor)
{
    std::vector<std::vector<float>> weights;
    weights.reserve(disks.size());
    for(Disk const & pi : disks)
    {
        weights.push_back(get_weight(pi, radii, diskfactor));
    }
    return weights;
}

Contribution compute_contribution(Disk const & pi, std::vector<Disk> const & others, std::vector<std::vector<float>> other_weights, std::vector<float> const & radii, std::vector<float> const & areas, float rmax, ASMCDD_params const & params, unsigned long same_category_index, unsigned long target_size, float diskfactor)
{
    auto nSteps = (unsigned long)(params.limit/params.step);
    Contribution out;
    out.pcf.resize(nSteps, 0);
    out.contribution.resize(nSteps, 0);
    out.weights.resize(nSteps);
    for(unsigned long k=0; k<nSteps; k++)
    {
        float perimeter = perimeter_weight(pi.x, pi.y, radii[k], diskfactor);
        out.weights[k] = perimeter <= 0 ? 0.0f : 1.f/perimeter;
    }
    if(others.empty())
        return out;
    for(unsigned long j=0; j<others.size(); j++)
    {
        if( j == same_category_index)
            continue;
        auto & pj = others[j];
        float d = diskDistance(pi, pj, rmax);
        for(unsigned long k=0; k<nSteps; k++)
        {
            float r = radii[k]/rmax;//diskDistance(pi, {pi.x+radii[k], pi.y, pj.r}, rmax);
            float res = gaussian_kernel(params.sigma, r-d);
            out.pcf[k]+= res;
            out.contribution[k]+= res*other_weights[j][k];
        }
    }
    for(unsigned long k=0; k<nSteps; k++)
    {
        out.pcf[k]*=out.weights[k]/areas[k];
        out.contribution[k] = out.pcf[k] + out.contribution[k]/areas[k];

        out.pcf[k]/=others.size();
        out.contribution[k]/=target_size;

    }

    return out;
}

std::vector<Target_pcf_type> compute_pcf(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, std::vector<float> const & area, std::vector<float> const & radii, float rmax, ASMCDD_params const & params)
{
    std::vector<Target_pcf_type> out;
    unsigned long nSteps = radii.size();
    out.resize(nSteps, {0,std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()});
    bool same_category = &disks_a == &disks_b;
    for(unsigned long i=0; i<disks_a.size(); i++)
    {
        auto current = compute_density(disks_a[i], disks_b, area, radii, rmax, params, same_category ? i : disks_b.size(), disks_b.size());
        for(unsigned long k=0; k<nSteps; k++)
        {
            current[k]/=disks_b.size();
            out[k].mean+=current[k];
            if(current[k] > out[k].max)
            {
                out[k].max = current[k];
            }
            if(current[k] < out[k].min)
            {
                out[k].min = current[k];
            }
        }
    }
    for(unsigned long k=0; k<nSteps; k++)
    {
        out[k].mean/=disks_a.size();
        out[k].radius = radii[k]/rmax;
    }
    return out;
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
            float inner = max((r-0.5f)*rmax, 0.f);
            area[i] = M_PI*(outer*outer - inner*inner);
            radii[i] = r*rmax;
        }
        target_areas.insert(std::make_pair(parent, area));
        target_radii.insert(std::make_pair(parent, radii));

        auto & parent_disks = (*categories.get())[parent].target_disks;
        target_pcf.insert(std::make_pair(parent, compute_pcf(target_disks, parent_disks, area, radii, rmax, *params.get())));
    }
}

struct Compare{
    float val;
    unsigned long i;
    unsigned long j;
};
#pragma omp declare reduction(minimum : Compare : omp_out = omp_in.val < omp_out.val ? omp_in : omp_out)

float compute_error(Contribution const & contribution, std::vector<float> const & currentPCF, std::vector<Target_pcf_type> const & target)
{
    // new_mean
    float error_mean=0;
    float error_min=0;
    float error_max=0;
    for(unsigned long k=0; k<currentPCF.size(); k++)
    {
        error_mean = std::max((currentPCF[k]+contribution.contribution[k] - target[k].mean)/target[k].mean, error_mean);
    }
    for(unsigned long k=0; k<currentPCF.size(); k++)
    {
        error_max = std::max((contribution.pcf[k] - target[k].max)/target[k].max, error_max);
    }
    for(unsigned long k=0; k<currentPCF.size(); k++)
    {
        error_min = std::max((target[k].min - contribution.pcf[k])/target[k].min, error_min);
    }
    return error_mean+std::max(error_max, error_min);
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

void ASMCDD::normalize(float domainLength){
    for(auto & c : *categories.get())
    {
        c.normalize(domainLength);
    }
}

std::vector<float> compute_pretty_pcf(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, std::vector<float> const & radii, std::vector<float> const & area, float rmax, ASMCDD_params const & params, float diskfactor)
{
    std::vector<float> pcf, density;
    pcf.resize(radii.size(), 0);
    density.resize(radii.size());
    for(auto const & pi : disks_a)
    {
        auto weight = get_weight(pi, radii, diskfactor);
        std::fill(density.begin(), density.end(), 0);
        for(unsigned long k=0; k<radii.size(); k++)
        {
            for(auto const & pj : disks_b)
            {
                if(&pi != &pj)
                {
                    density[k]+=gaussian_kernel(params.sigma, (radii[k]-euclidian(pi, pj))/rmax);
                }
            }
            pcf[k]+=density[k]*(weight[k] > 4 ? 4 : weight[k])/disks_a.size();
        }
    }
    for(unsigned long k=0; k<pcf.size(); k++)
    {
        pcf[k]/=area[k]*disks_b.size();
    }
    return pcf;
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
    float diskfactor = 1/domainLength;
    //Get all the disks and info
    std::vector<Compute_status> compute_stats;
    for(auto & c : *categories.get())
    {
        compute_stats.push_back(c.getTargetComputeStatus());
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
    unsigned long nbPoints=0;
    for(unsigned long c=0; c<compute_stats.size(); c++)
    {
        nbPoints+=compute_stats[c].disks.size();
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

    totalRmax = computeRmax(nbPoints);
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
        plots.first.push_back(stat.disks);
        std::vector<float> pcf = compute_pretty_pcf(stat.disks, stat.disks, radii[c], area[c], stat.rmax, *params.get(), diskfactor);
        std::vector<std::pair<float, float>> plot;
        plot.resize(pcf.size());
        for(unsigned long k=0; k<radii[c].size(); k++)
        {
            plot[k].first = radii[c][k]/stat.rmax;
            plot[k].second = pcf[k];
        }
        plots.second.emplace_back(std::make_pair(c, c), plot);
        for(unsigned long other : stat.parents)
        {
            pcf = compute_pretty_pcf(stat.disks, compute_stats[other].disks, totalRadii, totalArea, totalRmax, *params.get(), diskfactor);
            for(unsigned long k=0; k<radii[c].size(); k++)
            {
                plot[k].second = pcf[k];
            }
            plots.second.emplace_back(std::make_pair(other, c), plot);
        }
    }

    return plots;
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
