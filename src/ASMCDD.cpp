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

float ClassInteraction::disk_distance(Disk const & a, Disk const & b, float rmax){
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

//Returns the proportion of the circle perimeter that is in the [0, 1] domain
float perimeter_weight(float x, float y, float r)
{
    //We assume the domain is [0, 1] on all sides
    //Assuming the center x, y is in [0,1]
    float full_perimeter = float(2.0*M_PI)*r;
    float current_perimeter = full_perimeter;
    if(x+r > 1)
    {
        //Right bound intersection
        float i = std::acos(clip((1-x)/r, -1.0f, 1.0f));
        float max = std::atan2(1-y, 1-x);
        float min = std::atan2(-y, 1-x);
        current_perimeter -= (std::min(i, max) - std::max(-i, min))*r;
    }
    if(y+r > 1)
    {
        //Upper bound intersection
        float i = std::acos(clip((1-y)/r, -1.0f, 1.0f));
        float max = std::atan2(x, 1-y);
        float min = std::atan2(-(1-x), 1-y);
        current_perimeter -= (std::min(i, max) - std::max(-i, min))*r;
    }
    if(x < r)
    {
        //Left bound intersection
        float i = std::acos(clip(x/r, -1.0f, 1.0f));
        float max = std::atan2(y, x);
        float min = std::atan2(-(1-y), 1-x);
        current_perimeter -= (std::min(i, max) - std::max(-i, min))*r;
    }
    if(y < r)
    {
        //Lower bound intersection
        float i = std::acos(clip(y/r, -1.0f, 1.0f));
        float max = std::atan2((1-x), 1-y);
        float min = std::atan2(-x, 1-y);
        current_perimeter -= (std::min(i, max) - std::max(-i, min))*r;
    }
    return clip<float>(current_perimeter/full_perimeter, 0, 1);
}

std::vector<unsigned int> getTopologicalOrder(std::vector<ClassInteraction> const & target)
{
    std::map<unsigned int, std::vector<unsigned int>> nodes;

    for(auto && ci : target)
    {
        if(nodes.find(ci.class_a) == nodes.end())
        {
            nodes.insert(std::make_pair(ci.class_a, std::vector<unsigned int>()));
        }
        if(ci.class_a != ci.class_b){
            if(nodes.find(ci.class_b) == nodes.end())
            {
                nodes.insert(std::make_pair(ci.class_b, std::vector<unsigned int>()));
            }
            nodes[ci.class_a].push_back(ci.class_b);
        }
    }

    if(nodes.empty())
        return std::vector<unsigned int>();

    std::vector<unsigned int> degrees;
    degrees.resize(nodes.size(), 0);
    for(auto && n : nodes)
    {
        for(unsigned int e : n.second)
        {
            degrees[e]++;
        }
    }

    std::queue<unsigned int> q;
    for(unsigned long i=0; i<degrees.size();i++)
    {
        if(degrees[i]==0)
            q.push(i);
    }

    std::vector<unsigned int> top_order;
    while(!q.empty())
    {
        unsigned int u = q.front();
        q.pop();
        top_order.push_back(u);

        for(unsigned int e : nodes[u])
        {
            if(--degrees[e] == 0)
            {
                q.push(e);
            }
        }
    }
    return top_order;
}



void ClassInteraction::computePCF(std::vector<Disk> const &disks_a, std::vector<Disk> const &disks_b, float rmax, float step,
                                  float sigma, float limit, bool same_class){
    //We assume the disks are in a [0,1]^2 domain
    discardPCFs();

    int nSteps = int((limit/rmax)/step);

    meanPCF.resize(nSteps, 0);
    maxPCF.resize(nSteps, 0);
    minPCF.resize(nSteps, 0);
    radii.resize(nSteps);

    if(disks_a.empty() || disks_b.empty())
        return;


    std::vector<float> area;
    area.resize(nSteps);

    //Radii computation
    for(int i=0; i<nSteps; i++)
    {
        radii[i] = (i+1)*step;
    }

    //Area computation
    for(unsigned long i=0; i<radii.size(); i++)
    {
        float & r = radii[i];
        float outer = (r+0.5f)*rmax;
        float inner = max((r-0.5f)*rmax, 0.f);
        area[i] = M_PI*(outer*outer - inner*inner);
    }

    //Perimeter computation
    std::vector<std::vector<float>> perimeter;
    perimeter.resize(disks_a.size());
    for(unsigned long i=0; i<disks_a.size(); i++){
        perimeter[i].resize(radii.size());
        const Disk & d = disks_a[i];
        for(unsigned long j=0; j<radii.size(); j++)
        {
            perimeter[i][j] = perimeter_weight(d.x, d.y, radii[j]*rmax);
        }
    }


    std::vector<float> current_PCF;
    current_PCF.resize(radii.size());
    std::vector<float> sumContrib(radii.size());

    //Compute PCF
    for(unsigned long i=0; i<disks_a.size(); i++)
    {
        std::fill(current_PCF.begin(), current_PCF.end(), 0);
        std::fill(sumContrib.begin(), sumContrib.end(), 0);
        const Disk & pi = disks_a[i];
        for(unsigned long j=0; j<disks_b.size(); j++)
        {
            if(i == j && same_class) continue;
            const Disk & pj = disks_b[j];
            float d = disk_distance(pi, pj, rmax);
            for(unsigned long k=0; k<radii.size(); k++)
            {
                float r = disk_distance(pi, {pi.x+radii[k]*rmax, pi.y, pj.r}, rmax);
                float inner = std::max(r-0.5f, 0.f);//disk_distance(pi, {pi.x+(std::max(radii[k]-0.5f, 0.f)*rmax), pi.y, pj.r}, rmax);
                float outer = (r+0.5); //disk_distance(pi, {pi.x+(radii[k]+0.5f)*rmax, pi.y, pj.r}, rmax);
                if(perimeter[i][k] > 0)
                {
                    float a = area[k];
                    //a = M_PI*(outer*outer - inner*inner)/2;
                    float contrib = gaussian_kernel(sigma, r-d);
                    current_PCF[k]+=contrib/(a*perimeter[i][k]);
                }
            }
        }
        for(unsigned long k=0; k<radii.size(); k++)
        {
            current_PCF[k]/=disks_b.size();
            meanPCF[k]+=current_PCF[k];
            if(current_PCF[k] > maxPCF[k])
            {
                maxPCF[k]=current_PCF[k];
            }
            if(current_PCF[k] < minPCF[k])
            {
                minPCF[k]=current_PCF[k];
            }
        }
    }

    for(float & pcf : meanPCF)
    {
        pcf/=disks_a.size();
    }
}

void ClassInteraction::discardPCFs(){
    meanPCF.clear();
    maxPCF.clear();
    minPCF.clear();
    radii.clear();
}

ASMCDD::ASMCDD(std::string const &filename){
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
    disks.resize(n_classes);
    for(unsigned int i=0; i<n_classes; i++)
    {
        file >> buffer;
        id_map.insert_or_assign(buffer, i);
    }
    while(file >> buffer)
    {
        float x, y, r;
        unsigned int index = id_map[buffer];
        file >> x >> y >>r;
        x/=10000.f;
        y/=10000.f;
        r/=10000.f;
        disks[index].push_back({x,y,r});
    }
    file.close();
}

unsigned int ASMCDD::addClass(std::vector<Disk> const & _disks){
    disks.push_back(_disks);
    return disks.size()-1;
}

unsigned int ASMCDD::addClassDependency(unsigned int a, unsigned int b){
    interaction_graph.emplace_back(a,b);
    topological_order_computed=false;
    return interaction_graph.size()-1;
}

void ASMCDD::save(){
    disks_saved = disks;
    interaction_saved = interaction_graph;
}

void ASMCDD::computePCF(){
    /*unsigned long n=0;
    for(auto & di : disks)
    {
        n+=di.size();
    }*/
    for(auto & interaction : interaction_graph)
    {
        bool same_class = interaction.class_a == interaction.class_b;
        unsigned long n = same_class ? disks[interaction.class_a].size() : disks[interaction.class_a].size()+disks[interaction.class_b].size();
        float rmax = 2.0*std::sqrt(1/(2*std::sqrt(3)*n));
        interaction.computePCF(disks[interaction.class_a], disks[interaction.class_b], rmax, 0.1, 0.25, 5*rmax, same_class);
    }
}

std::vector<ClassInteraction> &ASMCDD::getSavedInteractions(){
    return interaction_saved;
}

std::vector<std::vector<Disk>> &ASMCDD::getSavedDisks(){
    return disks_saved;
}

void ASMCDD::setTargetPCFs(unsigned long nClasses, std::vector<ClassInteraction> const & _target){
    disks.clear();
    disks_saved.clear();
    interaction_graph.clear();
    interaction_saved.clear();

    disks.resize(nClasses);
    interaction_graph = _target;
    std::for_each(interaction_graph.begin(), interaction_graph.end(), [](ClassInteraction & i){i.discardPCFs();});

    target_graph = _target;
}

float computeError(ClassInteraction const & current, ClassInteraction const & target)
{
    float error=0;
    for(unsigned long i=0; i<current.meanPCF.size() && i<target.meanPCF.size(); i++)
    {
        error = std::max(error, current.meanPCF[i] - target.meanPCF[i]);
    }
    return error;
}

float computeError(std::vector<Target_pcf_type> const & current, std::vector<Target_pcf_type> const & target)
{
    float error=0;
    for(unsigned long i=0; i<current.size() && i<target.size(); i++)
    {
        error = std::max(error, current[i].mean - target[i].mean);
    }
    return error;
}

void ASMCDD::initialisation(float domainLength, float e_delta){
    static std::random_device rand_device;
    static std::mt19937_64 rand_gen(rand_device());
    static std::uniform_real_distribution<float> randf(0, 1);

    disks.clear();
    disks.resize(target_disks.size());

    if(!topological_order_computed){
        topological_order = getTopologicalOrder(target_graph);
        topological_order_computed=true;
    }


    //Init radii for each class
    std::vector<std::vector<float>> output_disks_radii;
    float n_factor = domainLength*domainLength;
    float diskfact = 1/domainLength;
    unsigned long long n_repeat = std::ceil(n_factor);

    for(auto& target : target_disks)
    {
        std::vector<float> disks_radii;
        disks_radii.reserve(n_repeat*target.size());
        for(auto & d : target)
        {
            for(unsigned long long i=0; i<n_repeat; i++)
            {
                disks_radii.push_back(d.r*diskfact);
            }
        }
        std::shuffle(disks_radii.begin(), disks_radii.end(), rand_gen); //Shuffle array
        disks_radii.resize(target.size()*n_factor); // and resize it to the number of disks we want
        // This combination effectively does a random non repeating sampling

        std::sort(disks_radii.rbegin(), disks_radii.rend()); //Sort the radii in descending order
        output_disks_radii.push_back(disks_radii);
    }

    //Attempt dart throw
    float e_0 = 0;
    unsigned long max_fails=1000;
    for(unsigned long class_i=0; class_i < output_disks_radii.size(); class_i++)
    {
        unsigned int cur_class = topological_order[class_i];
        disks[cur_class].reserve(output_disks_radii[cur_class].size());
        unsigned long fails=0;
        unsigned long n_accepted=0;
        do{
            bool accepted=false, rejected=false;
            float e = e_0 + e_delta*fails;
            disks[cur_class].emplace_back(randf(rand_gen), randf(rand_gen), output_disks_radii[cur_class][n_accepted]);
            for(auto & ci : interaction_graph)
            {
                if(ci.class_a == cur_class || ci.class_b == cur_class)
                {
                    bool same_class = ci.class_a == ci.class_b;
                    unsigned long n = same_class ? disks[ci.class_a].size() : disks[ci.class_a].size()+disks[ci.class_b].size();
                    float rmax = 2.0*std::sqrt(1/(2*std::sqrt(3)*n));
                    ci.computePCF(disks[ci.class_a], disks[ci.class_b],rmax,0.1, 0.25, 5*rmax, same_class);
                    for(auto & t_ci : target_graph)
                    {
                        if(t_ci.class_a == ci.class_a && t_ci.class_b == ci.class_b)
                        {
                            if(e > computeError(ci, t_ci))
                            {
                                accepted=true;
                            }else{
                                rejected=true;
                            }
                            break;
                        }
                    }
                }
                if(rejected)
                    break;
            }
            if(accepted){
                n_accepted++;
                fails = 0;
            }
            if(rejected){
                disks[cur_class].pop_back();
                fails++;
            }

            if(fails >= max_fails)
            {
                //grid search
                while(n_accepted < output_disks_radii[cur_class].size())
                {
                    int min_i=0, min_j=0;
                    float min_error=INFINITY;
                    for(int i=0; i<=100; i++)
                    {
                        for(int j=0; j<100; j++)
                        {
                            disks[cur_class].emplace_back(i*0.01, j*0.01, output_disks_radii[cur_class][n_accepted]);
                            for(auto & ci : interaction_graph)
                            {
                                if(ci.class_a == cur_class || ci.class_b == cur_class)
                                {
                                    bool same_class = ci.class_a == ci.class_b;
                                    unsigned long n = same_class ? disks[ci.class_a].size() : disks[ci.class_a].size()+disks[ci.class_b].size();
                                    float rmax = 2.0*std::sqrt(1/(2*std::sqrt(3)*n));
                                    ci.computePCF(disks[ci.class_a], disks[ci.class_b],rmax,0.1, 0.25, 5*rmax, same_class);
                                    for(auto & t_ci : target_graph)
                                    {
                                        if(t_ci.class_a == ci.class_a && t_ci.class_b == ci.class_b)
                                        {
                                            e = computeError(ci, t_ci);
                                            if(e < min_error)
                                            {
                                                min_error=e;
                                                min_i=i;
                                                min_j=j;
                                            }
                                        }
                                    }
                                }
                                if(rejected)
                                    break;
                            }
                            disks[cur_class].pop_back();
                        }
                    }
                    disks[cur_class].emplace_back(min_i*0.01, min_j*0.01, output_disks_radii[cur_class][n_accepted]);
                    n_accepted++;

                }
            }

        }while(n_accepted < output_disks_radii[cur_class].size());
    }

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

std::vector<Target_pcf_type> computePCF(std::vector<Disk> const & current, std::vector<Disk> const & other, std::vector<std::vector<float>> const & disks_weight, std::vector<float> const & areas, float rmax, ASMCDD_params const & params)
{
    bool same_category = &current == &other;
    auto nSteps = (unsigned long)(params.limit/params.step);
    std::vector<Target_pcf_type> out;
    out.resize(nSteps, {0,0,0});

    for(unsigned long i=0; i<nSteps; i++)
    {
        out[i].radius = (i+1)*params.step;
    }
    std::vector<float> currentPCF;
    currentPCF.resize(nSteps);
    for(unsigned long i=0; i<current.size(); i++)
    {
        std::fill(currentPCF.begin(), currentPCF.end(), 0);
        const Disk & pi = current[i];
        for(unsigned long j=0; j<other.size(); j++)
        {
            if(i == j && same_category) continue;
            const Disk & pj = other[j];
            float d = diskDistance(pi, pj, rmax);
            for(unsigned long k=0; k<out.size(); k++)
            {
                float r = diskDistance(pi, {pi.x+out[k].radius*rmax, pi.y, pj.r}, rmax);
                currentPCF[k]+=disks_weight[i][k]*gaussian_kernel(params.sigma, r-d)/areas[k];
            }
        }
        for(unsigned long k=0; k<out.size(); k++)
        {
            currentPCF[k]/=other.size();
            out[k].mean+=currentPCF[k];
            if(currentPCF[k] > out[k].max)
            {
                out[k].max=currentPCF[k];
            }
            if(currentPCF[k] < out[k].min)
            {
                out[k].max=currentPCF[k];
            }
        }
    }

    for(auto & pcf : out)
    {
        pcf.mean/=current.size();
    }

    return out;
}

void Category::computeTarget(){
    target_pcf.clear();
    target_rmax.clear();

    if(target_disks.empty())
    {
        return;
    }

    auto nSteps = (unsigned long)(params->limit/params->step);

    target_rmax.insert(std::make_pair(id, computeRmax(target_disks.size())));
    for(unsigned long parent : parents_id)
    {
        target_rmax.insert(std::make_pair(parent, computeRmax(target_disks.size() + (*categories.get())[parent].target_disks.size())));
    }
    std::vector<float> area;
    area.resize(nSteps);
    float rmax=target_rmax[id];
    for(unsigned long i=0; i<nSteps; i++)
    {
        float r = (i+1)*rmax;
        float outer = (r+0.5f)*rmax;
        float inner = max((r-0.5f)*rmax, 0.f);
        area[i] = M_PI*(outer*outer - inner*inner);
    }
    target_areas.insert(std::make_pair(id, area));
    for(unsigned long parent : parents_id)
    {
        rmax = target_rmax[parent];
        std::vector<std::vector<float>> weights;
        weights.reserve(target_disks.size());

        for(unsigned long i=0; i<target_disks.size(); i++)
        {
            weights.emplace_back();
            for(unsigned long j=0; j<nSteps; j++)
            {
                float perimeter = perimeter_weight(target_disks[i].x, target_disks[i].y, (j+1)*rmax);
                weights[i].push_back(perimeter <= 0 ? 0.0f : 1.f/perimeter);
            }
        }

        for(unsigned long i=0; i<nSteps; i++)
        {
            float r = (i+1)*rmax;
            float outer = (r+0.5f)*rmax;
            float inner = max((r-0.5f)*rmax, 0.f);
            area[i] = M_PI*(outer*outer - inner*inner);
        }
        target_areas.insert(std::make_pair(parent, area));

        std::vector<Disk> const & parent_disks = (*categories.get())[parent].target_disks;
        target_pcf.insert(std::make_pair(parent, computePCF(target_disks, parent_disks, weights, area, rmax, *(params.get()))));
    }
}

void Category::initialize(float domainLength, float e_delta){
    if(initialized)
        return;
    static std::random_device rand_device;
    static std::mt19937_64 rand_gen(rand_device());
    static std::uniform_real_distribution<float> randf(0, 1);

    disks.clear();

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
            output_disks_radii.push_back(d.r*diskfact);
        }
    }
    std::shuffle(output_disks_radii.begin(), output_disks_radii.end(), rand_gen); //Shuffle array
    output_disks_radii.resize(target_disks.size()*n_factor); // and resize it to the number of disks we want
    // This combination effectively does a random non repeating sampling

    std::sort(output_disks_radii.rbegin(), output_disks_radii.rend()); //Sort the radii in descending order

    float e_0 = 0;
    unsigned long max_fails=1000;
    unsigned long fails=0;
    unsigned long n_accepted=0;
    disks.reserve(output_disks_radii.size());
    std::vector<std::vector<float>> disks_weights;
    disks_weights.reserve(output_disks_radii.size());
    unsigned long nSteps = params->limit/params->step;
    do{
        bool rejected=false;
        float e = e_0 + e_delta*fails;
        disks.emplace_back(randf(rand_gen), randf(rand_gen), output_disks_radii[n_accepted]);
        disks_weights.emplace_back();
        for(unsigned long j=0; j<nSteps; j++)
        {
            float perimeter = perimeter_weight(disks.back().x, disks.back().y, (j+1)*target_rmax[id]);
            disks_weights.back().push_back(perimeter <= 0 ? 0.0f : 1.f/perimeter);
        }
        auto current_pcf = computePCF(disks, disks, disks_weights, target_areas[id], target_rmax[id], *(params.get()));
        if(e < computeError(current_pcf, target_pcf[id])){
            rejected=true;
        }else{
            for(auto parent : parents_id)
            {
                current_pcf = computePCF(disks, (*categories.get())[parent].disks, disks_weights, target_areas[id], target_rmax[id], *(params.get()));
                if(e < computeError(current_pcf, target_pcf[parent]))
                {
                    rejected=true;
                    break;
                }
            }
        }
        if(rejected)
        {
            disks.pop_back();
            disks_weights.pop_back();
            fails++;
        }else
        {
            n_accepted++;
            fails=0;
        }
        if(fails > max_fails)
        {
            //Grid search
            while(n_accepted < output_disks_radii.size())
            {
                unsigned long min_i=0, min_j=0;
                float minError=INFINITY;
                float currentError=0;
                for(unsigned long i=0; i<=100; i++)
                {
                    for(unsigned long j=0; j<=100; j++)
                    {
                        disks.emplace_back(0.01*i, 0.01*j, output_disks_radii[n_accepted]);
                        disks_weights.emplace_back();
                        for(unsigned long k=0; k<nSteps; k++)
                        {
                            float perimeter = perimeter_weight(disks.back().x, disks.back().y, (k+1)*target_rmax[id]);
                            disks_weights.back().push_back(perimeter <= 0 ? 0.0f : 1.f/perimeter);
                        }
                        auto current_pcf = computePCF(disks, disks, disks_weights, target_areas[id], target_rmax[id], *(params.get()));
                        currentError = computeError(current_pcf, target_pcf[id]);
                        for(auto parent : parents_id)
                        {
                            current_pcf = computePCF(disks, (*categories.get())[parent].disks, disks_weights, target_areas[id], target_rmax[id], *(params.get()));
                            currentError = std::max(currentError, computeError(current_pcf, target_pcf[parent]));
                        }
                        disks.pop_back();
                        disks_weights.pop_back();
                        if(currentError < minError)
                        {
                            minError = currentError;
                            min_i=i;
                            min_j=j;
                        }
                    }
                }
                disks.emplace_back(0.01*min_i, 0.01*min_j, output_disks_radii[n_accepted]);
                disks_weights.emplace_back();
                for(unsigned long k=0; k<nSteps; k++)
                {
                    float perimeter = perimeter_weight(disks.back().x, disks.back().y, (k+1)*target_rmax[id]);
                    disks_weights.back().push_back(perimeter <= 0 ? 0.0f : 1.f/perimeter);
                }
                n_accepted++;
            }

        }
    }while(n_accepted < output_disks_radii.size());
    pcf.insert_or_assign(id, computePCF(disks, disks, disks_weights, target_areas[id], target_rmax[id], *(params.get())));
    for(auto parent : parents_id)
    {
        pcf.insert_or_assign(parent, computePCF(disks, (*categories.get())[parent].disks, disks_weights, target_areas[id], target_rmax[id], *(params.get())));
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
    return disks;
}

unsigned long ASMCDD_new::addTargetClass(std::vector<Disk> const &target){
    categories->emplace_back(categories->size(), categories, params);
    categories->back().setTargetDisks(target);
    return categories->size()-1;
}

Category &ASMCDD_new::getClass(unsigned long id){
    return categories->at(id);
}

void ASMCDD_new::setParams(ASMCDD_params const &_params){
    (*params.get()) = _params;
}

void ASMCDD_new::addDependency(unsigned long parent, unsigned long child){
    categories->at(parent).addChild(child);
    categories->at(child).addDependency(parent);
}

void ASMCDD_new::computeTarget(){
    for(auto & category : (*categories.get()))
    {
        category.computeTarget();
    }
}

std::vector<Target_pcf_type> ASMCDD_new::getTargetPCF(unsigned long parent, unsigned long child){
    return categories->at(child).getTargetPCF(parent);
}

std::vector<Target_pcf_type> ASMCDD_new::getCurrentPCF(unsigned long parent, unsigned long child){
    return categories->at(child).getCurrentPCF(parent);
}

std::vector<Target_pcf_type> ASMCDD_new::getTargetPCF(unsigned long self){
    return getTargetPCF(self, self);
}

std::vector<Target_pcf_type> ASMCDD_new::getCurrentPCF(unsigned long self){
    return getCurrentPCF(self, self);
}

void ASMCDD_new::initialize(float domainLength, float e_delta){
    for(auto & category : (*categories.get()))
    {
        category.initialize(domainLength, e_delta);
    }
}

std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> ASMCDD_new::getCurrentPCFplot(){
    std::vector<std::pair<std::pair<unsigned long, unsigned long>, std::vector<std::pair<float, float>>>> result;
    for(auto & category : (*categories.get()))
    {
        auto value = category.getCurrentPCFs();
        result.reserve(result.size()+value.size());
        result.insert(result.end(), value.begin(), value.end());
    }
    return result;
}

std::vector<Disk> ASMCDD_new::getCurrentDisks(unsigned long id){
    return categories->at(id).getCurrentDisks();
}
