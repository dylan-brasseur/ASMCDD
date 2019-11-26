//
// Created by "Dylan Brasseur" on 22/11/2019.
//

#ifndef DISKSPROJECT_GRAPH_H
#define DISKSPROJECT_GRAPH_H

#include <string>
#include <vector>
#include <map>
#include "Vec3.h"

struct Plot{
    std::string name;
    float color[3];
    std::vector<std::pair<float,float>> dataPoints;
    bool operator==(std::string const & str)
    {
        return name==str;
    }
};

class Graph{
public:
    enum POSITION{TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, CENTER};
    Graph();
    Graph(const std::string & xaxis_name, const std::string & yaxis_name);

    void clearDataPoints(std::string const & name);
    void clear();
    unsigned int addPlot(const std::string & name);
    unsigned int addDataPoint(unsigned int plot_id, std::pair<float, float> const & point);
    unsigned int addDataPoints(unsigned int plot_id, std::vector<std::pair<float, float>> const & points);
    void modifyPoint(unsigned int plot, unsigned int index, std::pair<float, float> const & point);
    void modifyPoints(unsigned int plot, unsigned int start_index, std::vector<std::pair<float, float>> const & point);
    void removeDataPoints(unsigned int plot_id, unsigned int start, unsigned int end);

    [[nodiscard]] float3 getPlotColor(unsigned int plot_id) const;
    void setPlotColor(unsigned int, float3 const & color);

    void draw(GLint plot_VBO_location, GLint bounds_uniform_location, float* bounds, GLint color_location, bool axis_on=true);
    void drawLegend(Graph::POSITION position = TOP_RIGHT){throw implementation_error();}
    void buildGraphVBO();

    void setAxisTickSpacing(float tick);
    void setAxisTickSize(float size);
private:
    void buildRawVBO();
    std::vector<Plot> plots;
    bool pointsChanged, axis_changed;
    unsigned long long int bufferSize;
    unsigned long long int currentSize;

    std::string x_axis_name, y_axis_name;
    float tick_spacing, tick_length;

    GLuint plot_VAO_id, plot_VBO_id;
    std::vector<float> plot_VBO;
};

#endif //DISKSPROJECT_GRAPH_H
