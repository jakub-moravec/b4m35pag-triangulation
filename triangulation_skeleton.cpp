#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <stdint.h>



using namespace std;
using namespace std::chrono;

// Structure of a point in 2D plane
struct Point {
	float x, y;
};

unsigned long p = 0;

float calculate_distance(float x, float x1, float y, float y1);
void get_triags(const vector<vector<int>> &min_triags, vector<tuple<int, int, int>> &triangles, int i, int j);

float INF = std::numeric_limits<float>::max();



// A Dynamic programming based function to find minimum cost for convex polygon triangulation.
tuple<vector<tuple<int, int, int>>, float> triangulate(vector<Point> points) {

	float distances[p][p] = {0};
	float costs[p][p] = {0};
    std::vector<std::vector<int> > min_triags(p, std::vector<int>(p, -1));

#pragma omp parallel for simd schedule(guided)
	for (int i = 0; i < p; ++i)
		for (int j = 0; j <= i; ++j)
			distances[j][i] = distances[i][j] = calculate_distance(points[i].x, points[i].y, points[j].x, points[j].y);

	for (int diff = 0; diff < p; ++diff) {
#pragma omp parallel for schedule(guided) num_threads(24) if(p > 1000)
		for (int j = diff; j < p; ++j) {
			int i = j - diff;
			if (j > i + 1) {
				float min_cost = INF;
				float ji_dist =  distances[j][i];
#pragma omp simd reduction(min : min_cost)
				for (int k = i + 1; k < j; ++k) {
					float cost = costs[i][k] + costs[j][k] + distances[i][k] + distances[j][k] + ji_dist;
					if (cost < min_cost) {
                        min_triags[i][j] = k;
						min_cost = cost;
					}
				}
				costs[i][j] = min_cost;
				costs[j][i] = min_cost;
			} else {
				costs[i][j] = 0;
				costs[j][i] = 0;
			}
		}
	}


	vector<tuple<int, int, int>> triangles;
    get_triags(min_triags, triangles, 0, p - 1);
	return make_tuple(move(triangles), costs[0][p-1]);
}

void get_triags(const vector<vector<int>> &min_triags, vector<tuple<int, int, int>> &triangles, int i, int j) {
    int k = min_triags[i][j];
    if(k > -1) {
        triangles.push_back(make_tuple(i, j, k));
        get_triags(min_triags, triangles, i, k);
        get_triags(min_triags, triangles, k, j);
    }
}

float calculate_distance(float x1, float x2, float y1, float y2) {
	return sqrt((pow(x1-y1, 2)) + (pow(x2 - y2, 2)));
}

vector<Point> readProblem(const string& inputFile)	{
	vector<Point> points;
	ifstream bin(inputFile.c_str(), ifstream::binary);
	if (bin)	{
		int32_t n = 0;
		bin.read((char*) &n, sizeof(int32_t));
		for (uint64_t p = 0; p < n; ++p)	{
			float x = 0.0, y = 0.0;
			bin.read((char*) &x, sizeof(float));
			bin.read((char*) &y, sizeof(float));
			points.push_back({x, y});
		}

		bin.close();
	} else {
		throw invalid_argument("Cannot open the input file '"+inputFile+"' to read the problem.");
	}
	p = points.size();
	return points;
}

void writeResult(float triagCost, const vector<tuple<int, int, int>>& triangles, const string& resultFile)	{
	ofstream bout(resultFile.c_str(), ofstream::binary | ofstream::trunc);
	if (bout)	{
		bout.write((char*) &triagCost, sizeof(float));
		for (const auto& triangle : triangles)	{
			int32_t p1, p2, p3;
			tie(p1, p2, p3) = triangle;
			bout.write((char*) &p1, sizeof(int32_t));
			bout.write((char*) &p2, sizeof(int32_t));
			bout.write((char*) &p3, sizeof(int32_t));
		}

		bout.close();
	} else {
		throw invalid_argument("Cannot write the results, check the permissions.");
	}
}

void writeImage(const vector<Point>& points, const vector<tuple<int, int, int>>& triangles, const string& imageFilename)	{
	constexpr uint32_t numOfColors = 10;
	array<string, numOfColors> colors = {
		"orange", "brown", "purple", "blue", "darksalmon", "yellow", "green", "red", "lime", "aqua"
	};

	float minX = 10e6, maxX = -10e6, minY = 10e6, maxY = -10e6;
	for (const Point& p : points)	{
		minX = min(minX, p.x);
		maxX = max(maxX, p.x);
		minY = min(minY, p.y);
		maxY = max(maxY, p.y);
	}

	float mult = 1600.0f/(maxX-minX), height = ceil(mult*(maxY-minY));

	ofstream im(imageFilename);
	if (im)	{
		im<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1600\" height=\""<<height<<"\">"<<endl;

		default_random_engine generator;
		uniform_int_distribution<uint32_t> colorIdx(0u, numOfColors-1u);
		for (const tuple<int, int, int>& t : triangles)	{
			int i, j, k;
			tie(i, j, k) = t;

			array<Point, 3> p { points[i], points[j], points[k] };
			for (uint32_t i = 0; i < 3; ++i)	{
				p[i].x = mult*(p[i].x-minX);
				p[i].y = mult*(p[i].y-minY);
			}

			im<<"\t<polygon points=\""<<p[0].x<<","<<p[0].y<<" "<<p[1].x<<","<<p[1].y<<" "<<p[2].x<<","<<p[2].y<<"\" ";
			im<<"style=\"fill:"<<colors[colorIdx(generator)]<<";stroke:black;stroke-width=0.3\"/>"<<endl;
		}

		for (uint32_t i = 0; i < points.size(); ++i)	{
			array<Point, 2> p { points[i % points.size()], points[(i+1) % points.size()] };
			for (uint32_t i = 0; i < 2; ++i)	{
				p[i].x = mult*(p[i].x-minX);
				p[i].y = mult*(p[i].y-minY);
			}	

			im<<"\t<line x1=\""<<p[0].x<<"\" y1=\""<<p[0].y<<"\" x2=\""<<p[1].x<<"\" y2=\""<<p[1].y<<"\" ";
			im<<"stroke-width=\"2\" stroke=\"black\"/>"<<endl;
		}

		im<<"</svg>"<<endl;
		im.close();
	} else {
		cerr<<"Cannot write the result to svg file!"<<endl;
	}
}

int main(int argc, char* argv[]) {
	string imageFilename, inputFile, resultFile;
	for (int i = 1; i < argc; ++i)	{
		string parg = argv[i];
		if (parg == "--help" || parg == "-h")	{
			cout<<"Triangulation of a convex polygon."<<endl;
			cout<<endl<<"Usage:"<<endl;
			cout<<"\t"<<argv[0]<<" input_problem.bin result.bin [options]"<<endl<<endl;
			cout<<"General options:"<<endl;
			cout<<"\t--output-image ARG, -of ARG, ARG: {FILE_NAME}.svg"<<endl;
			cout<<"\t\tThe output svg file demonstrating the triangulation."<<endl;
			cout<<"\t--help, -h"<<endl;
			cout<<"\t\tIt prints this help."<<endl;
			return 0;
		}

		if (parg == "--output-image" || parg == "-of")	{
			if (i+1 < argc)	{
				imageFilename = argv[++i];
			} else {
				cerr<<"Expected a filename for the output image!"<<endl;
				return 1;
			}
		}

		if (!parg.empty() && parg[0] != '-')	{
			if (inputFile.empty())
				inputFile = parg;
			else
				resultFile = parg;
		}
	}

	try {
		high_resolution_clock::time_point start = high_resolution_clock::now();

		float criterion;
		vector<tuple<int, int, int>> triangles;
		const vector<Point>& points = readProblem(inputFile);

		tie(triangles, criterion) = triangulate(points);
		double totalDuration = duration_cast<duration<double>>(high_resolution_clock::now()-start).count();

		if (!resultFile.empty())
			writeResult(criterion, triangles, resultFile);

		if (!imageFilename.empty())
			writeImage(points, triangles, imageFilename);

		cout<<"Cost of triangulation: "<<criterion<<endl;
		cout<<"computational time: "<<totalDuration<<" s"<<endl;

	} catch (exception& e) {
		cerr<<"Exception caught: "<<e.what()<<endl;
		return 2;
	}

	return 0;
}

