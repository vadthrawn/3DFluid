namespace octet
{
	namespace shaders
	{
		class color_shader : public shader
		{
			GLuint modelToProjectionIndex_;

		public:
			void init()
			{
				#ifdef OCTET_VITA
					shader::init_bin(cg_shaderbin_color_v, cg_shaderbin_color_f);

				#else
					const char vertex_shader[] = SHADER_STR(
						attribute vec4 pos;
						uniform mat4 modelToProjection;
						
						void main()
						{
							gl_Position = modelToProjection * pos;
						}
					);

					const char fragment_shader[] = SHADER_STR(
						uniform vec4 emissive_color;
						void main()
						{
							gl_FragColor = emissive_color;
						}
					);
    
					shader::init(vertex_shader, fragment_shader);
				#endif

				modelToProjectionIndex_ = glGetUniformLocation(program(), "modelToProjection");
			}

			void render(const mat4t &modelToProjection, const vec4 &emissive_color)
			{
				shader::render();

				glUniformMatrix4fv(modelToProjectionIndex_, 1, GL_FALSE, modelToProjection.get());
			}
		};
	}
}