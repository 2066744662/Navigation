/*
    Name: Robbie Feng
    SID: 1668372
    CCID: Tianjun2
    CMPUT 275, Winter 2021-2022
    Assignment: Trivial Navigation System - part2
*/
#include <iostream>
#include <cassert>
#include <fstream>
#include <string>
#include <list>
#include <vector>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wdigraph.h"
#include "dijkstra.h"

#define MSG_SIZE 1024

struct Point
{
	long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point &pt1, const Point &pt2)
{
	long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
	return abs(dLat) + abs(dLon);
}

// finds the id of the point that is closest to the given point "pt"
int findClosest(const Point &pt, const unordered_map<int, Point> &points)
{
	pair<int, Point> best = *points.begin();

	for (const auto &check : points)
	{
		if (manhattan(pt, check.second) < manhattan(pt, best.second))
		{
			best = check;
		}
	}
	return best.first;
}

// read the graph from the file that has the same format as the "Edmonton graph" file
void readGraph(const string &filename, WDigraph &g, unordered_map<int, Point> &points)
{
	ifstream fin(filename);
	string line;

	while (getline(fin, line))
	{
		// split the string around the commas, there will be 4 substrings either way
		string p[4];
		int at = 0;
		for (auto c : line)
		{
			if (c == ',')
			{
				// start new string
				++at;
			}
			else
			{
				// append character to the string we are building
				p[at] += c;
			}
		}

		if (at != 3)
		{
			// empty line
			break;
		}

		if (p[0] == "V")
		{
			// new Point
			int id = stoi(p[1]);
			assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
			points[id].lat = static_cast<long long>(stod(p[2]) * 100000);
			points[id].lon = static_cast<long long>(stod(p[3]) * 100000);
			g.addVertex(id);
		}
		else
		{
			// new directed edge
			int u = stoi(p[1]), v = stoi(p[2]);
			g.addEdge(u, v, manhattan(points[u], points[v]));
		}
	}
}

int create_and_open_fifo(const char *pname, int mode)
{
	// creating a fifo special file in the current working directory
	// with read-write permissions for communication with the plotter
	// both proecsses must open the fifo before they can perform
	// read and write operations on it
	if (mkfifo(pname, 0666) == -1)
	{
		cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
		exit(-1);
	}

	// opening the fifo for read-only or write-only access
	// a file descriptor that refers to the open file description is
	// returned
	int fd = open(pname, mode);

	if (fd == -1)
	{
		cout << "Error: failed on opening named pipe." << endl;
		exit(-1);
	}

	return fd;
}

void array2point(char *line, Point *sp, Point *ep)
{
	/*Records the array receive from inpipe into two Point class objects
	Input:
		char*: the array to convert
		Point*: Start point to recode coordinates
		Point*: End point to recode coordinates.
	*/
	vector<double> vec;
	string tempstr = "";
	int i = 0;
	while (vec.size() < 4) {
		if (line[i] == ' ' || line[i] == '\n')
		{
			vec.push_back(stod(tempstr) * 100000);
			tempstr = "";
		}
		else
		{
			tempstr += line[i];
		}
		i++;
	}
	sp->lat = static_cast<long long>(vec[0]);
	sp->lon = static_cast<long long>(vec[1]);
	ep->lat = static_cast<long long>(vec[2]);
	ep->lon = static_cast<long long>(vec[3]);
	return;
}

// keep in mind that in part 1, the program should only handle 1 request
// in part 2, you need to listen for a new request the moment you are done
// handling one request
int main()
{
	WDigraph graph;
	unordered_map<int, Point> points;

	const char *inpipe = "inpipe";
	const char *outpipe = "outpipe";

	// Open the two pipes

	int in = create_and_open_fifo(inpipe, O_RDONLY);
	cout << "inpipe opened..." << endl;
	int out = create_and_open_fifo(outpipe, O_WRONLY);
	cout << "outpipe opened..." << endl;

	// build the graph
	readGraph("server/edmonton-roads-2.0.1.txt", graph, points);

	// read a request
	while (true)
	{
		Point sPoint, ePoint;
		char line[MSG_SIZE] = {0};
		int bytes_read = read(in, line, MSG_SIZE);
        //Exit program
        if (line[0] == 'Q') {
            close(in);
			close(out);
			unlink(inpipe);
			unlink(outpipe);
            return 0;
		}

		array2point(line, &sPoint, &ePoint);
		  /*cout << sPoint.lat << " " << sPoint.lon << " " << ePoint.lat << " " << ePoint.lon << endl;//delete me
			char c;
			cin >> c;*/

		// get the points closest to the two points we read
		int start = findClosest(sPoint, points), end = findClosest(ePoint, points);

		// run dijkstra's algorithm, this is the unoptimized version that
		// does not stop when the end is reached but it is still fast enough
		unordered_map<int, PIL> tree;
		dijkstra(graph, start, tree);

		// NOTE: in Part II you will use a different communication protocol than Part I
		// So edit the code below to implement this protocol

		// have path
		if (tree.find(end) != tree.end())
		{
			// read off the path by stepping back through the search tree
			list<int> path;
			while (end != start)
			{
				path.push_front(end);
				end = tree[end].first;
			}
			path.push_front(start);

			// output the path
			for (int v : path) {
				double outlat = static_cast<double>(points[v].lat) / 100000;
				double outlon = static_cast<double>(points[v].lon) / 100000;
				/*cout << outlat << ' ' << outlon << endl;*/
				string tempstr = to_string(outlat) + " " + to_string(outlon) + "\n";
				char outLine[tempstr.length()];
				for (int i = 0; i < sizeof(outLine); i++)
				{
					outLine[i] = tempstr[i];
				}

				write(out, outLine, sizeof outLine);
			}
		}
		char outLine[2] = {'E', '\n'};
		write(out, outLine, sizeof outLine);
	}
	return 0;
}
