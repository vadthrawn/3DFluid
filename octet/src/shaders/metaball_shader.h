namespace octet
{
	namespace shaders
	{
		class metaball_shader : public shader
		{
			GLuint modelToProjection_, emissive_color_, position_, threshold_;

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
						uniform vec4 emissive_color;
						uniform vec3 position;
						uniform float threshold;

						void main()
						{
							//vec3 currentPixel = gl_FragCoord.xyz;
							//float magnitude = sqrt (pow (currentPixel.x - position.x, 2.0) + pow (currentPixel.y - position.y, 2.0) + pow (currentPixel.z - position.z, 2.0));
							
							//gl_FragColor = vec4(0, 0, 0, 1);

							//if (magnitude > threshold)
								gl_FragColor = emissive_color;
						}
					);
    
					shader::init (vertex_shader, fragment_shader);
				#endif
				
				modelToProjection_ = glGetUniformLocation (program(), "modelToProjection");
				emissive_color_ = glGetUniformLocation (program(), "emissive_color");
				position_ = glGetUniformLocation (program(), "position");
				threshold_ = glGetUniformLocation (program(), "threshold");
			}

			void render (const mat4t &modelToProjection, const vec4 &emissive_color, const vec3 &position, const float &threshold)
			{
				shader::render();

				glUniformMatrix4fv (modelToProjection_, 1, GL_FALSE, modelToProjection.get());
				glUniform4fv (emissive_color_, 1, emissive_color.get());
				glUniform4fv (position_, 1, position.get());
				glUniform1f (threshold_, threshold);
			}
		};
	}
}