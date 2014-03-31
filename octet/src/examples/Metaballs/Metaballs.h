namespace octet
{
	class Metaballs : public app
	{
		metaball_shader* mb_shader;
		int* mb_positions;
		float* pixels;
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

			mb_threshold = 50;
			mb_color = vec4(1, 1, 1, 1);

			numOfMetaballs = 3;
			mb_positions = new int[numOfMetaballs * 2];
			mb_positions[0] = 200;
			mb_positions[1] = 300;
			mb_positions[2] = 500;
			mb_positions[3] = 600;
			mb_positions[4] = 300;
			mb_positions[5] = 400;
			
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

			mb_shader->render (modelToProjection, mb_color, mb_positions, mb_threshold, numOfMetaballs);

			glEnableVertexAttribArray (attribute_position);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glVertexAttribPointer (attribute_position, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (GLfloat), 0);
			glDrawArrays (GL_POINTS, 0, sizeOfPixels);

			if (is_key_down ('W'))
				mb_positions[1] += 1;

			if (is_key_down ('S'))
				mb_positions[1] -= 1;

			if (is_key_down ('A'))
				mb_positions[0] -= 1;

			if (is_key_down ('D'))
				mb_positions[0] += 1;
		}
	};
}