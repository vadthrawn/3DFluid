namespace octet
{
	class Metaballs : public app
	{
		metaball_shader* mb_shader;
		float* mb_positions;
		int numOfMetaballs;
		vec4 mb_color;
		float mb_threshold;
		GLuint positionBuffer, attribute_position;
		mat4t modelToWorld, cameraToWorld;

	public:
		Metaballs(int argc, char **argv) : app(argc, argv) { }

		~Metaballs ()
		{
			delete[] mb_positions;
		}

		/// this is called once OpenGL is initialized
		void app_init()
		{
			mb_shader = new metaball_shader;
			mb_shader->init();

			attribute_position = glGetAttribLocation (mb_shader->program(), "pos");

			mb_threshold = 0.5f;
			mb_color = vec4(1, 1, 1, 1);

			numOfMetaballs = 2;

			mb_positions = new float[numOfMetaballs * 3];
			
			mb_positions[0] = 1.5f;
			mb_positions[1] = 1.0f;
			mb_positions[2] = 0.0f;
			mb_positions[3] = 3.0f;
			mb_positions[4] = 2.0f;
			mb_positions[5] = 0.0f;
			
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

			dynarray<float> points;

			for (float i = -5.00f; i <= 5.00f; i = i + 0.01f)
			{
				for (float j = -5.00f; j <= 5.00f; j = j + 0.01f)
				{
					float potential = 0.0f;

					for (int m = 0; m < numOfMetaballs; m++)
					{
						float magnitude = sqrt (pow (i - mb_positions[(m * 3)], 2) + pow (j - mb_positions[(m * 3) + 1], 2));
						potential += mb_threshold / magnitude;
					}

					if (potential > 1.0f)
					{
						points.push_back (i);
						points.push_back (j);
						points.push_back (0.0f);
					}
				}
			}

			glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable (GL_DEPTH_TEST);

			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld, 0.5f, 256.0f);

			mb_shader->render (modelToProjection, mb_color, mb_positions[0], mb_threshold);

			glGenBuffers (1, &positionBuffer);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			float* metaball_Positions = new float[points.size()];

			for (int i = 0; i < (int)points.size(); i++)
			{
				metaball_Positions[i] = points[i];
			}

			glBufferData (GL_ARRAY_BUFFER, points.size() * sizeof (GLfloat), metaball_Positions, GL_STATIC_DRAW);

			glEnableVertexAttribArray (attribute_position);
			glVertexAttribPointer (attribute_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GL_FLOAT), 0);
			glDrawArrays (GL_POINTS, 0, points.size());
			glDisableVertexAttribArray (attribute_position);

			glDeleteBuffers (1, &positionBuffer);

			if (is_key_down ('W'))
				mb_positions[1] += 0.01f;

			if (is_key_down ('S'))
				mb_positions[1] -= 0.01f;

			if (is_key_down ('A'))
				mb_positions[0] -= 0.01f;

			if (is_key_down ('D'))
				mb_positions[0] += 0.01f;
		}
	};
}