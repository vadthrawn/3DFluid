namespace octet
{
	class Metaballs
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
		Metaballs() { }

		~Metaballs ()
		{
			delete[] mb_positions;
			delete[] velocities;
			delete[] colors;
			delete mb_shader;
		}

		/// this is called once OpenGL is initialized
		void InitMetaballs()
		{
			mb_shader = new metaball_shader;
			mb_shader->init();

			const float cameraTranslate = 5.0f;

			attribute_position = glGetAttribLocation (mb_shader->program(), "pos");
			int i = 0;

			float vertices[] = {
				-cameraTranslate, -cameraTranslate, (float)i,
				cameraTranslate, cameraTranslate, (float)i,
				cameraTranslate, -cameraTranslate, (float)i,

				-cameraTranslate, -cameraTranslate, (float)i,
				-cameraTranslate, cameraTranslate, (float)i,
				cameraTranslate, cameraTranslate, (float)i++,
			};

			glGenBuffers (1, &positionBuffer);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

			mb_threshold = 20;
			
			modelToWorld.loadIdentity();
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, cameraTranslate);
		}

		/// this is called to draw the world
		void renderMetaballs()
		{
			//int vx, vy;
			
			//get_viewport_size (vx, vy);
			//glViewport (0, 0, vx, vy);

			//glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
			//glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			//glEnable (GL_DEPTH_TEST);

			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld, 0.5f, 256.0f);

			mb_shader->render (modelToProjection, colors, mb_positions, mb_threshold, numOfMetaballs);

			glEnableVertexAttribArray (attribute_position);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glVertexAttribPointer (attribute_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GLfloat), 0);

			int size;
			glGetBufferParameteriv (GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

			glDrawArrays (GL_TRIANGLES, 0, size / sizeof (GLfloat));

			//for (int i = 0; i < numOfMetaballs; i++)
				//AdjustPosition (mb_positions[i * 3], mb_positions[i * 3 + 1], mb_positions[i * 3 + 2], velocities[i * 3], velocities[i * 3 + 1], velocities[i * 3 + 2]);

			if (is_key_down ('W'))
				mb_positions[1] += 1;

			if (is_key_down ('S'))
				mb_positions[1] -= 1;

			if (is_key_down ('A'))
				mb_positions[0] -= 1;

			if (is_key_down ('D'))
				mb_positions[0] += 1;
		}

		/*
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
		*/

		void UpdateMetaballs (float* pos, const int &size, const int &vx, const int &vy)
		{
			numOfMetaballs = size;
			mb_positions = new int[numOfMetaballs * 2];
			//velocities = new float[numOfMetaballs * 3];
			colors = new float[numOfMetaballs * 3];

			for (int i = 0; i < numOfMetaballs; i++)
			{
				printf("Particle %i - X: %f, Y: %f\n", i, pos[i * 2] * vx, pos[i * 2 + 1] * vy);
				mb_positions[i * 2] = pos[i * 2] * vx;
				mb_positions[i * 2 + 1] = pos[i * 2 + 1] * vy;
				//mb_positions[i * 3 + 2] = 0;
				//velocities[0] = -5.0f;
				//velocities[1] = -3.0f;
				//velocities[2] = 0.0f;
				colors[i * 3] = 0.0f;
				colors[i * 3 + 1] = 0.2f;
				colors[i * 3 + 2] = 0.5f;
			}
		}
	};
}