//
// Created by "Dylan Brasseur" on 22/11/2019.
//

#include "../include/Graph.h"
#include <algorithm>
Graph::Graph() {
    plot_VAO_id = plot_VBO_id = bufferSize = currentSize = 0;
    pointsChanged = false;
    axis_changed = false;
    tick_spacing = 1;
    tick_length = 0.1;
}
Graph::Graph(const std::string &xaxis_name, const std::string &yaxis_name) : Graph(){
    x_axis_name = xaxis_name;
    y_axis_name = yaxis_name;
}

void Graph::clearDataPoints(std::string const &name){
    auto entry = std::find(plots.begin(), plots.end(), name);
    if(entry != plots.end())
    {
        currentSize-=entry->dataPoints.size();
        entry->dataPoints.clear();
    }
}

void Graph::clear(){
    plots.clear();
}

unsigned int Graph::addPlot(const std::string &name){
    Plot plot;
    plot.name = name;
    plot.color[0]=plot.color[1]=plot.color[2]=0;
    plots.push_back(std::move(plot));
    return plots.size()-1;
}

unsigned int Graph::addDataPoint(unsigned int plot_id, std::pair<float, float> const &point){
    plots.at(plot_id).dataPoints.push_back(point);
    pointsChanged=true;
    currentSize++;
    return plots[plot_id].dataPoints.size()-1;
}

unsigned int Graph::addDataPoints(unsigned int plot_id, std::vector<std::pair<float, float>> const &points){
    auto plot = plots.at(plot_id);
    plot.dataPoints.reserve(plot.dataPoints.size()+points.size());
    for(auto & p : points)
    {
        plot.dataPoints.push_back(p);
    }
    currentSize+=points.size();
    pointsChanged=true;
    return plot.dataPoints.size()-points.size();
}

void Graph::modifyPoint(unsigned int plot, unsigned int index, std::pair<float, float> const &point){
    plots.at(plot).dataPoints.at(index) = point;
    pointsChanged = true;
}

void Graph::removeDataPoints(unsigned int plot_id, unsigned int start, unsigned int end){
    auto data = plots.at(plot_id).dataPoints;
    data.erase(data.begin()+start, data.begin()+end);
    currentSize-= (end-start);
    pointsChanged = true;
}

float3 Graph::getPlotColor(unsigned int plot_id) const{
    return {plots.at(plot_id).color[0], plots.at(plot_id).color[1], plots.at(plot_id).color[2]};
}

void Graph::setPlotColor(unsigned int plot_id, float3 const &color){
    Plot & p = plots.at(plot_id);
    p.color[0] = color.a;
    p.color[1] = color.b;
    p.color[2] = color.c;
}

void Graph::buildGraphVBO(){
    if(plot_VAO_id == 0)
    {
        glGenVertexArrays(1, &plot_VAO_id);
    }
    if(plot_VBO_id == 0)
    {
        glGenBuffers(1, &plot_VBO_id);
    }
    if(pointsChanged)
    {
        buildRawVBO();
        glBindBuffer(GL_ARRAY_BUFFER, plot_VBO_id);TEST_OPENGL_ERROR();
        if(currentSize > bufferSize/2)
        {
            bufferSize = plot_VBO.size();
            glBufferData(GL_ARRAY_BUFFER, bufferSize*sizeof(float), plot_VBO.data(), GL_DYNAMIC_DRAW);TEST_OPENGL_ERROR();
        }else
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, plot_VBO.size()*sizeof(float), plot_VBO.data());TEST_OPENGL_ERROR();
        }
    }

}

void Graph::buildRawVBO(){
    //Reserve 2 * number of points
    plot_VBO.clear();
    plot_VBO.reserve(2*currentSize);

    //Copy plots in the VBO
    for(auto & p : plots)
    {
        for(auto & point : p.dataPoints)
        {
            plot_VBO.push_back(point.first);
            plot_VBO.push_back(point.second);
        }
    }
    pointsChanged=false;
}

void Graph::setAxisTickSpacing(float spacing){
    tick_spacing = spacing;
}

void Graph::setAxisTickSize(float size){
    tick_length = size;
}

void Graph::draw(GLint plot_VBO_location, GLint bounds_uniform_location, float *bounds, GLint color_location, bool axis_on){
    buildGraphVBO();
    glUniform4fv(bounds_uniform_location, 1, bounds);TEST_OPENGL_ERROR();
    glBindVertexArray(plot_VAO_id);TEST_OPENGL_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, plot_VBO_id);TEST_OPENGL_ERROR();
    glVertexAttribPointer(plot_VBO_location, 2, GL_FLOAT, false, 0, nullptr);TEST_OPENGL_ERROR();
    glEnableVertexAttribArray(plot_VBO_location);TEST_OPENGL_ERROR();
    unsigned int index=0;
    std::for_each(plots.begin(), plots.end(), [&](Plot p){
        glUniform3fv(color_location, 1, p.color);TEST_OPENGL_ERROR();
        glDrawArrays(GL_LINE_STRIP, index, p.dataPoints.size());TEST_OPENGL_ERROR();
        index+=p.dataPoints.size();
    });
    glBindVertexArray(0);
}

void Graph::modifyPoints(unsigned int plot, unsigned int start_index, std::vector<std::pair<float, float>> const &point){
    std::copy(point.begin(), point.end(), plots.at(plot).dataPoints.begin()+start_index);
    pointsChanged = true;
}




