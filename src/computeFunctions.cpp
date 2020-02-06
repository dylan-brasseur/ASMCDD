//
// Created by "Dylan Brasseur" on 06/02/2020.
//


#include "../include/computeFunctions.h"

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
        density[k]*=weights[k]/areas[k];
    }
    return density;
}



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
    out.weights.resize(nSteps, 1);
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
            float r = radii[k]/rmax;
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

std::vector<Target_pcf_type> compute_pcf(std::vector<Disk> const & disks_a, std::vector<Disk> const & disks_b, std::vector<float> const & area, std::vector<float> const & radii, float rmax, ASMCDD_params const & params){
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
