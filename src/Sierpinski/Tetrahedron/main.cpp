#include <gproshan/app_viewer.h>

#include <vector>

namespace gp = gproshan;

class sierpinski3d: public gp::app_viewer
{
	std::vector<gp::vec3> points;
	
	public:
		sierpinski3d(const unsigned n = 10000000)
		{
			points.reserve(n);
			
			// 4 vértices para formar un tetraedro PERFECTAMENTE equilátero
			points.push_back({ 1.f,  1.f,  1.f});
			points.push_back({ 1.f, -1.f, -1.f});
			points.push_back({-1.f,  1.f, -1.f});
			points.push_back({-1.f, -1.f,  1.f});
			
			sierpinski_3d(n);
		}

		int main()
		{
			init();
			run();
			return 0;
		}

		void init() override
		{
			add_mesh(new gp::che(points.data(), size(points), nullptr, 0));
			run();
		}
		
		void sierpinski_3d(const unsigned n)
		{
			float a = rand() % 10000;
			float b = rand() % 10000;
			float c = rand() % 10000;
			float d = rand() % 10000; // Un peso más para el 4to vértice

			// Punto inicial aleatorio usando coordenadas baricéntricas 3D
			gp::vec3 p = (a * points[0] + b * points[1] + c * points[2] + d * points[3]) / (a + b + c + d);
			
			while(size(points) < n)
			{
				// En lugar de % 3, ahora es % 4 porque tenemos 4 vértices
				points.push_back((p + points[rand() % 4]) / 2.f);
				p = points.back();
			}
		}
};

int main()
{
	std::srand(std::time({}));

	sierpinski3d app;
	return app.main();
}
