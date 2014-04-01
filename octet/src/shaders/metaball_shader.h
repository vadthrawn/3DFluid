namespace octet
{
	namespace shaders
	{
		class metaball_shader : public shader
		{
			GLuint modelToProjection_, emissive_color_, position_, threshold_, numOfMetaballs_;

		public:
			void init ()
			{
				#ifdef OCTET_VITA
					shader::init_bin(cg_shaderbin_color_v, cg_shaderbin_color_f);

				#else
					const char vertex_shader[] = SHADER_STR(
						attribute vec3 pos;
						uniform mat4 modelToProjection;

						void main()
						{
							gl_Position = modelToProjection * vec4 (pos, 1);
						}
					);

					const char fragment_shader[] = SHADER_STR(
						const int maxNumOfMetaballs = 20;

						uniform vec3 emissive_color[maxNumOfMetaballs];
						uniform int numOfMetaballs;
						uniform int positions[2 * maxNumOfMetaballs];
						uniform int threshold;

						void main()
						{
							float potential = 0.0;
							vec3 currentColor = vec3 (0, 0, 0);

							for (int i = 0; i < numOfMetaballs; i++)
							{
								float magnitude = sqrt (pow (gl_FragCoord.x - positions[i * 2], 2.0) + pow (gl_FragCoord.y - positions[i * 2 + 1], 2.0));
								float currentBallPotential = threshold / magnitude;

								potential += currentBallPotential;
								currentColor = mix (currentColor, emissive_color[i], currentBallPotential);
							}
							
							gl_FragColor = vec4(0, 0, 0, 1);

							if (potential > 1.0)
								gl_FragColor = vec4 (currentColor, 1);
						}
					);
    
					shader::init (vertex_shader, fragment_shader);
				#endif
				
				modelToProjection_ = glGetUniformLocation (program(), "modelToProjection");
				emissive_color_ = glGetUniformLocation (program(), "emissive_color");
				position_ = glGetUniformLocation (program(), "positions");
				threshold_ = glGetUniformLocation (program(), "threshold");
				numOfMetaballs_ = glGetUniformLocation (program(), "numOfMetaballs");
			}

			void render (const mat4t &modelToProjection, const float* emissive_color, const int* position, const int &threshold, const int &numOfMetaballs)
			{
				shader::render();

				glUniform1i (numOfMetaballs_, numOfMetaballs);
				glUniformMatrix4fv (modelToProjection_, 1, GL_FALSE, modelToProjection.get());
				glUniform3fv (emissive_color_, numOfMetaballs, emissive_color);
				glUniform1iv (position_, numOfMetaballs * 2, position);
				glUniform1i (threshold_, threshold);
			}
		};
	}
}