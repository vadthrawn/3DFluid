namespace octet
{
	class Metaballs : public app
	{
		metaball_shader* mb_shader;
		int* mb_positions;
		float* pixels;
		vec2* velocities;
		float* colors;
		int sizeOfPixels;
		int numOfMetaballs;
		vec4 mb_color;
		int mb_threshold;
		GLuint positionBuffer, attribute_position;
		mat4t modelToWorld, cameraToWorld;

	public:
		Metaballs(int argc, char **argv) : app(argc, argv) { }

		~Metaballs ()
		{
			delete[] mb_positions;
			delete[] pixels;
			delete[] velocities;
			delete[] colors;
			delete mb_shader;
		}

		/// this is called once OpenGL is initialized
		void app_init()
		{
			mb_shader = new metaball_shader;
			mb_shader->init();

			attribute_position = glGetAttribLocation (mb_shader->program(), "pos");

			sizeOfPixels = 2000000;
			pixels = new float[sizeOfPixels];
			float xIndex = -5.00f;

			for (int i = 0; i < 1000; i++)
			{
				float yIndex = -5.00f;

				for (int j = 0; j < 1000; j++)
				{
					pixels[(i * 1000 * 2) + (j * 2) + 0] = xIndex;
					pixels[(i * 1000 * 2) + (j * 2) + 1] = yIndex;

					yIndex += 0.01f;
				}

				xIndex += 0.01f;
			}

			glGenBuffers (1, &positionBuffer);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glBufferData (GL_ARRAY_BUFFER, sizeOfPixels * sizeof (GLfloat), pixels, GL_STATIC_DRAW);

			mb_threshold = 30;
			mb_color = vec4(1, 1, 1, 1);

			numOfMetaballs = 6;
			mb_positions = new int[numOfMetaballs * 2];
			velocities = new vec2[numOfMetaballs];
			colors = new float[numOfMetaballs * 3];

			mb_positions[0] = 200;
			mb_positions[1] = 300;
			velocities[0] = vec2 (2.0f, 3.0f);
			colors[0] = 1.0f;
			colors[1] = 0.0f;
			colors[2] = 0.0f;


			mb_positions[2] = 500;
			mb_positions[3] = 600;
			velocities[1] = vec2 (-1.0f, 2.5f);
			colors[3] = 0.0f;
			colors[4] = 1.0f;
			colors[5] = 0.0f;

			mb_positions[4] = 300;
			mb_positions[5] = 400;
			velocities[2] = vec2 (5.0f, -7.5f);
			colors[6] = 0.0f;
			colors[7] = 0.0f;
			colors[8] = 1.0f;

			mb_positions[6] = 600;
			mb_positions[7] = 650;
			velocities[3] = vec2 (-5.0f, 7.5f);
			colors[9] = 1.0f;
			colors[10] = 1.0f;
			colors[11] = 0.0f;

			mb_positions[8] = 50;
			mb_positions[9] = 100;
			velocities[4] = vec2 (2.5f, 5.0f);
			colors[12] = 0.0f;
			colors[13] = 1.0f;
			colors[14] = 1.0f;

			mb_positions[10] = 300;
			mb_positions[11] = 400;
			velocities[5] = vec2 (-3.0f, -1.0f);
			colors[15] = 1.0f;
			colors[16] = 1.0f;
			colors[17] = 1.0f;
			
			modelToWorld.loadIdentity();
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0.0f, 0.0f, 5.0f);
		}

		/// this is called to draw the world
		void draw_world(int x, int y, int w, int h)
		{
			int vx, vy;

			get_viewport_size (vx, vy);
			glViewport (0, 0, vx, vy);

			glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable (GL_DEPTH_TEST);

			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld, 0.5f, 256.0f);

			mb_shader->render (modelToProjection, colors, mb_positions, mb_threshold, numOfMetaballs);

			glEnableVertexAttribArray (attribute_position);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glVertexAttribPointer (attribute_position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (GLfloat), 0);
			glDrawArrays (GL_POINTS, 0, sizeOfPixels);

			for (int i = 0; i < numOfMetaballs; i++)
				AdjustPosition (mb_positions[i * 2], mb_positions[i * 2 + 1], velocities[i]);

			/*
			if (is_key_down ('W'))
				mb_positions[1] += 1;

			if (is_key_down ('S'))
				mb_positions[1] -= 1;

			if (is_key_down ('A'))
				mb_positions[0] -= 1;

			if (is_key_down ('D'))
				mb_positions[0] += 1;
				*/
		}

		void CheckBoundries (int &positionX, int &positionY, vec2 &velocity)
		{
			int vx, vy;
			get_viewport_size (vx, vy);

			if (positionX < mb_threshold)
			{
				positionX = mb_threshold;
				velocity.x() = - velocity.x();
			}

			else if (positionX > vx - mb_threshold)
			{
				positionX = vx - mb_threshold;
				velocity.x() = -velocity.x();
			}

			if (positionY < mb_threshold)
			{
				positionY = mb_threshold;
				velocity.y() = -velocity.y();
			}

			else if (positionY > vy - mb_threshold)
			{
				positionY = vy - mb_threshold;
				velocity.y() = -velocity.y();
			}
		}

		void AdjustPosition (int &positionX, int &positionY, vec2 &velocity)
		{
			positionX += (int)velocity.x();
			positionY += (int)velocity.y();

			CheckBoundries (positionX, positionY, velocity);
		}
	};
}