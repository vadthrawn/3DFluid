namespace octet
{
	class Metaballs : public app
	{
		metaball_shader* mb_shader;
		int* mb_positions;
		float* velocities;
		float* colors;
		int numOfMetaballs;
		int mb_threshold;
		GLuint positionBuffer, attribute_position;
		mat4t modelToWorld, cameraToWorld;

	public:
		Metaballs(int argc, char **argv) : app(argc, argv) { }

		~Metaballs ()
		{
			delete[] mb_positions;
			delete[] velocities;
			delete[] colors;
			delete mb_shader;
		}

		/// this is called once OpenGL is initialized
		void app_init()
		{
			mb_shader = new metaball_shader;
			mb_shader->init();

			const float cameraTranslate = 5.0f;

			attribute_position = glGetAttribLocation (mb_shader->program(), "pos");

			float vertices[] = {
				-cameraTranslate, -cameraTranslate, 0.0f,
				cameraTranslate, cameraTranslate, 0.0f,
				cameraTranslate, -cameraTranslate, 0.0f,

				-cameraTranslate, -cameraTranslate, 0.0f,
				-cameraTranslate, cameraTranslate, 0.0f,
				cameraTranslate, cameraTranslate, 0.0f,
			};

			glGenBuffers (1, &positionBuffer);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

			mb_threshold = 20;

			numOfMetaballs = 2;
			mb_positions = new int[numOfMetaballs * 3];
			velocities = new float[numOfMetaballs * 3];
			colors = new float[numOfMetaballs * 3];

			mb_positions[0] = 200;
			mb_positions[1] = 300;
			mb_positions[2] = 0;
			velocities[0] = -5.0f;
			velocities[1] = -3.0f;
			velocities[2] = 0.0f;
			colors[0] = 0.0f;
			colors[1] = 0.2f;
			colors[2] = 0.5f;

			mb_positions[3] = 400;
			mb_positions[4] = 350;
			mb_positions[5] = 0;
			velocities[3] = 2.5f;
			velocities[4] = 4.0f;
			velocities[5] = 0.0f;
			colors[3] = 0.0f;
			colors[4] = 0.2f;
			colors[5] = 0.5f;

			/*
			mb_positions[4] = 300;
			mb_positions[5] = 400;
			velocities[2] = vec2 (5.0f, -7.5f);
			colors[6] = 0.0f;
			colors[7] = 0.0f;
			colors[8] = 1.0f;

			mb_positions[6] = 600;
			mb_positions[7] = 650;
			velocities[3] = vec2 (5.0f, 7.5f);
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

			mb_positions[12] = 175;
			mb_positions[13] = 100;
			velocities[6] = vec2 (-2.0f, 3.0f);
			colors[18] = 1.0f;
			colors[19] = 0.0f;
			colors[20] = 0.0f;

			mb_positions[14] = 450;
			mb_positions[15] = 700;
			velocities[7] = vec2 (-3.0f, -2.5f);
			colors[21] = 0.0f;
			colors[22] = 1.0f;
			colors[23] = 0.0f;

			mb_positions[16] = 100;
			mb_positions[17] = 100;
			velocities[8] = vec2 (5.0f, -7.5f);
			colors[24] = 0.0f;
			colors[25] = 0.0f;
			colors[26] = 1.0f;

			mb_positions[18] = 600;
			mb_positions[19] = 600;
			velocities[9] = vec2 (-5.0f, 7.5f);
			colors[27] = 1.0f;
			colors[28] = 1.0f;
			colors[29] = 0.0f;

			mb_positions[20] = 50;
			mb_positions[21] = 100;
			velocities[10] = vec2 (2.5f, -6.0f);
			colors[30] = 0.0f;
			colors[31] = 1.0f;
			colors[32] = 1.0f;

			mb_positions[22] = 500;
			mb_positions[23] = 400;
			velocities[11] = vec2 (-3.0f, 3.0f);
			colors[33] = 1.0f;
			colors[34] = 1.0f;
			colors[35] = 1.0f;
			*/
			
			modelToWorld.loadIdentity();
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0.0f, 0.0f, cameraTranslate);
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

			glVertexAttribPointer (attribute_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GLfloat), 0);

			int size;
			glGetBufferParameteriv (GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

			glDrawArrays (GL_TRIANGLES, 0, size / sizeof (GLfloat));

			for (int i = 0; i < numOfMetaballs; i++)
				AdjustPosition (mb_positions[i * 3], mb_positions[i * 3 + 1], mb_positions[i * 3 + 2], velocities[i * 3], velocities[i * 3 + 1], velocities[i * 3 + 2]);

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

		void CheckBoundries (int &positionX, int &positionY, int &positionZ, float &velocityX, float &velocityY, float &velocityZ)
		{
			int vx, vy;
			get_viewport_size (vx, vy);

			if (positionX < mb_threshold)
			{
				positionX = mb_threshold;
				velocityX = - velocityX;
			}

			else if (positionX > vx - mb_threshold)
			{
				positionX = vx - mb_threshold;
				velocityX = -velocityX;
			}

			if (positionY < mb_threshold)
			{
				positionY = mb_threshold;
				velocityY = -velocityY;
			}

			else if (positionY > vy - mb_threshold)
			{
				positionY = vy - mb_threshold;
				velocityY = -velocityY;
			}
		}

		void AdjustPosition (int &positionX, int &positionY, int &positionZ, float &velocityX, float &velocityY, float &velocityZ)
		{
			positionX += (int)velocityX;
			positionY += (int)velocityY;
			positionZ += (int)velocityZ;

			CheckBoundries (positionX, positionY, positionZ, velocityX, velocityY, velocityZ);
		}
	};
}