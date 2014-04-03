namespace octet
{
	static const int gridSize = 100;
	static const float baseSize = 5;

	class Isosurface : public app
	{
	public:
		struct Node
		{
			vec2 position;
			int numOfTriangles;
			byte corners;

			Node () : corners(0) { }
		};

		struct Circle
		{
			vec2 position;
			float radius;
			vec2 velocity;
		};

	private:
		mat4t modelToWorld, cameraToWorld;

		Node* nodes;
		Circle* circles;
		color_shader* cs;

		GLuint vbo, attribute_position;

		int numOfCircles;
		int numOfTriangles;
		float* color;

	public:
		Isosurface (int argc, char** argv) : app (argc, argv) {}
		~Isosurface ()
		{
			delete[] circles;
			delete[] nodes;
			delete[] color;
		}


		void app_init ()
		{
			cs = new color_shader();
			cs->init();

			color = new float[1];

			color[0] = 1.0f;
			color[1] = 1.0f;
			color[2] = 1.0f;
			color[3] = 1.0f;

			numOfCircles = 1;
			circles = new Circle[numOfCircles];

			circles[0].position = vec2 (-1.0f, -0.5f);
			circles[0].radius = 1.0f * (gridSize / baseSize);
			circles[0].velocity = vec2 (1.0f, 0.5f);
			
			nodes = new Node[gridSize * 2 * gridSize * 2];

			numOfTriangles = 0;

			for (int i = 0; i < gridSize * 2; i++)
			{
				for (int j = 0; j < gridSize * 2; j++)
					nodes[i * gridSize * 2 + j].position = vec2 (i - (float)gridSize + 0.5f, j - (float)gridSize + 0.5f);
			}

			attribute_position = glGetAttribLocation (cs->program(), "pos");

			glGenBuffers (1, &vbo);

			modelToWorld.loadIdentity();
			cameraToWorld.loadIdentity();

			cameraToWorld.translate (0.0f, 0.0f, (float)gridSize);
		}


		void draw_world (int x, int y, int h, int w)
		{
			glClearColor (0.2f, 0.2f, 0.2f, 1.0f);
			glClear (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			glEnable (GL_DEPTH_TEST);

			mat4t worldToCamera;
			cameraToWorld.invertQuick (worldToCamera);

			mat4t cameraToProjection;
			cameraToProjection.loadIdentity();
			cameraToProjection.frustum (-0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 256.0f);

			mat4t modelToProjection = modelToWorld * worldToCamera * cameraToProjection;

			cs->render(modelToProjection, color);

			for (int k = 0; k < numOfCircles; k++)
			{
				for (int i = 0; i < (gridSize * 2) * (gridSize * 2); i++)
				{
					nodes[i].corners = VerticesInBounds (nodes[i], circles[k]);
					nodes[i].numOfTriangles = FindNumOfTriangles (nodes[i]);
					numOfTriangles += nodes[i].numOfTriangles;
				}
			}

			float* vertices = new float[numOfTriangles * 12];
			int j = 0;

			for (int i = 0; i < numOfCircles; i++)
			{
				Triangulize (circles[i], j, vertices);
				circles[i].position.x() += circles[i].velocity.x();
				circles[i].position.y() += circles[i].velocity.y();
				CheckBoundries (circles[i]);
			}

			glBindBuffer (GL_ARRAY_BUFFER, vbo);
			glBufferData (GL_ARRAY_BUFFER, j * sizeof (GLfloat), vertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray (attribute_position);

			int size;
			glGetBufferParameteriv (GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

			glVertexAttribPointer (attribute_position, 4, GL_FLOAT, GL_FALSE, 4 * sizeof (GLfloat), 0);
			glDrawArrays (GL_TRIANGLES, 0, numOfTriangles * 3);

			delete[] vertices;
		}

	private:
		byte VerticesInBounds (Node &node, const Circle &circle) const
		{
			byte numOfCorners = 0;

			vec4 magnitude = vec4 (
				sqrt (pow (circle.position.x() - node.position.x() + 0.5f, 2) + pow (circle.position.y() - node.position.y() + 0.5f, 2)),
				sqrt (pow (circle.position.x() - node.position.x() + 0.5f, 2) + pow (circle.position.y() - node.position.y() - 0.5f, 2)),
				sqrt (pow (circle.position.x() - node.position.x() - 0.5f, 2) + pow (circle.position.y() - node.position.y() + 0.5f, 2)),
				sqrt (pow (circle.position.x() - node.position.x() - 0.5f, 2) + pow (circle.position.y() - node.position.y() - 0.5f, 2))
				);

			if (magnitude.x() < circle.radius)
				numOfCorners |= 1;
				
			if (magnitude.y() < circle.radius)
				numOfCorners |= 2;

			if (magnitude.z() < circle.radius)
				numOfCorners |= 4;

			if (magnitude.w() < circle.radius)
				numOfCorners |= 8;

			return numOfCorners;
		}

		int FindNumOfTriangles (const Node &node) const
		{
			switch (node.corners)
			{
				case 0:
					return 0;
					break;

				case 1:
				case 2:
				case 4:
				case 8:
					return 1;
					break;

				case 3:
				case 5:
				case 6:
				case 9:
				case 10:
				case 12:
				case 15:
					return 2;
					break;

				case 7:
				case 11:
				case 13:
				case 14:
					return 3;
					break;

				default:
					return -1;
					break;
			}
		}


		void CheckBoundries (Circle &circle)
		{
			int vx, vy;
			vx = vy = gridSize;

			if (circle.position.x() < -vx + circle.radius)
			{
				circle.position.x() = -vx + circle.radius;
				circle.velocity.x() = -circle.velocity.x();
			}

			else if (circle.position.x() > vx - circle.radius)
			{
				circle.position.x() = vx - circle.radius;
				circle.velocity.x() = -circle.velocity.x();
			}

			if (circle.position.y() < -vy + circle.radius)
			{
				circle.position.y() = - vy + circle.radius;
				circle.velocity.y() = -circle.velocity.y();
			}

			else if (circle.position.y() > vy - circle.radius)
			{
				circle.position.y() = vy - circle.radius;
				circle.velocity.y() = -circle.velocity.y();
			}
		}


		vec2 FindInterpolatedCoordinate (const vec2 &vertex1, const vec2 &vertex2, const Circle &circle)
		{
			vec2 interpolatedValue;
			float magnitude = pow (circle.radius, 2);

			if (vertex1.x() == vertex2.x())
			{
				interpolatedValue.x() = vertex1.x();
				interpolatedValue.y() = sqrt (magnitude - pow (vertex1.x() - circle.position.x(), 2)) + circle.position.y();

				if (vertex1.y() <= 0 && vertex2.y() <= 0)
					interpolatedValue.y() = -interpolatedValue.y();
			}

			else
			{
				interpolatedValue.y() = vertex1.y();
				interpolatedValue.x() = sqrt (magnitude - pow (vertex1.y() - circle.position.y(), 2)) + circle.position.x();

				if (vertex1.x() <= 0 && vertex2.x() <= 0)
					interpolatedValue.x() = -interpolatedValue.x();
			}

			return interpolatedValue;
		}


		void Triangulize (const Circle &circle, int &j, float* vertices)
		{
			for (int i = 0; i < ((gridSize * 2) * (gridSize * 2)); i++)
			{
				if (nodes[i].numOfTriangles <= 0)
					continue;

				switch (nodes[i].corners)
				{
				case 0:
					break;

				case 1:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 2:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 3:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 4:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() + 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 5:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y());
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 6:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate2 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate3 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate4 (nodes[i].position.x() + 0.5f, nodes[i].position.y());

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate3.x();
						vertices[j++] = interpolatedCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate4.x();
						vertices[j++] = interpolatedCoordinate4.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 7:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 cornerCoordinate3 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y());
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate3.x();
						vertices[j++] = cornerCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 8:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() + 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 9:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate3 (nodes[i].position.x() + 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate4 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate3.x();
						vertices[j++] = interpolatedCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate4.x();
						vertices[j++] = interpolatedCoordinate4.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 10:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y());
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 11:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 cornerCoordinate3 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() + 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate3.x();
						vertices[j++] = cornerCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 12:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 13:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate3 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x(), nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate2 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), circle);

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate3.x();
						vertices[j++] = cornerCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 14:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate3 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);
						vec2 interpolatedCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y());
						vec2 interpolatedCoordinate2 (nodes[i].position.x(), nodes[i].position.y() - 0.5f);
						//vec2 interpolatedCoordinate1 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f), circle);
						//vec2 interpolatedCoordinate2 = FindInterpolatedCoordinate (vec2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f), vec2 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f), circle);

						vertices[j++] = cornerCoordinate3.x();
						vertices[j++] = cornerCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate3.x();
						vertices[j++] = cornerCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate1.x();
						vertices[j++] = interpolatedCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate3.x();
						vertices[j++] = cornerCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = interpolatedCoordinate2.x();
						vertices[j++] = interpolatedCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}

				case 15:
					{
						vec2 cornerCoordinate1 (nodes[i].position.x() - 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate2 (nodes[i].position.x() - 0.5f, nodes[i].position.y() + 0.5f);
						vec2 cornerCoordinate3 (nodes[i].position.x() + 0.5f, nodes[i].position.y() - 0.5f);
						vec2 cornerCoordinate4 (nodes[i].position.x() + 0.5f, nodes[i].position.y() + 0.5f);

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate2.x();
						vertices[j++] = cornerCoordinate2.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate4.x();
						vertices[j++] = cornerCoordinate4.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate1.x();
						vertices[j++] = cornerCoordinate1.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate4.x();
						vertices[j++] = cornerCoordinate4.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;

						vertices[j++] = cornerCoordinate3.x();
						vertices[j++] = cornerCoordinate3.y();
						vertices[j++] = 0.0f;
						vertices[j++] = 1.0f;
						break;
					}
				}
			}
		}
	};
}