
namespace octet
{
	struct Vec2 { 
	float x,y; 
	Vec2() :x(0),y(0) { } 
	Vec2(float a, float b) : x(a), y(b) { }
	Vec2 operator+(const Vec2& b) const { return Vec2(x+b.x, y+b.y); }
	Vec2 operator-(const Vec2& b) const { return Vec2(x-b.x, y-b.y); }
	Vec2 & operator=(const Vec2& b) { x=b.x; y=b.y; return *this; }
	Vec2 & operator+=(const Vec2& b) { return *this = *this + b; }
	Vec2 & operator-=(const Vec2& b) { return *this = *this - b; }

	float operator*(const Vec2& b) const { return x*b.x + y*b.y; }
	Vec2 operator*(float b) const { return Vec2(x * b, y * b); }
	Vec2 operator/(float b) const { return Vec2(x / b, y / b); }
	float len2() const { return *this * *this; }
	float len() const { return sqrt(len2()); }
	Vec2 normal() const { return *this / len(); }
	};
	Vec2 operator*(float b, const Vec2& a) { return Vec2(a.x * b, a.y * b); }


	//structure for holding two neihgbouring particles and distance between them
	struct Neighbour
	{
		int i, j;
		float dist;
		float dist2;
	};

	//particle structure
	struct Particle
	{
		Particle():
		position(Vec2()), 
		position_old(Vec2()), 
		velocity(Vec2()),
		force(Vec2()),
		mass(0.0f),
		rho(0.0f),
		rho_near(0.0f),
		pressure(0.0f),
		pressure_near(0.0f),
		sigma(0.0f),
		beta(0.0f)
		{

		}
		Vec2 position;
		Vec2 position_old;
		Vec2 velocity;
		Vec2 force;

		float mass;
		float rho;
		float rho_near;
		float pressure;
		float pressure_near;
		float sigma;
		float beta;

		//dynarray<Neighbour> m_neighbours; //array of all neighbours of this particle
		std::vector<Neighbour> m_neighbours;
	};

	const int num_of_particles = 100;
	float gravity = .02f * .25;
	float spacing = 2.0f; //spacing of particles
	float k_far = spacing/10; //far pressure weight
	float k_near = k_far*10; //near pressure weight
	float rest_density = 3;
	float radius = spacing * 1.25f; //radius of support
	float radius_sq = radius*radius; //squared radius for performance reasons
	float sim_w = 50; //size of the simulation world
	float bottom = 0; //floor of the world

	std::vector<Particle> m_particles;
	//dynarray<Particle> m_particles; //all particles
	//sph_solver solver;

	class SPH_Fluid_System : public app
	{	
		mat4t modelToWorld;
		mat4t cameraToWorld;
		color_shader color_shader_;
		float angle;
		metaball_shader shader;
		int mb_threshold, numOfMetaballs;
		float* colors, *mb_positions;
		GLuint positionBuffer, attribute_position;
		
	public:
		SPH_Fluid_System(int argc, char **argv) : app(argc, argv) { }

		~SPH_Fluid_System ()
		{
			delete[] colors;
			delete[] mb_positions;
		}

		/// this is called once OpenGL is initialized
		void app_init()
		{
			// initialize the shader
			//color_shader_.init();
			shader.init();

			float vertices[] = {
				-1.0f, -1.0f, 0.0f,
				 1.0f, -1.0f, 0.0f,
				 -1.0f,  1.0f, 0.0f,

				1.0f, -1.0f, 0.0f,
				-1.0f, 1.0f, 0.0f,
				1.0f, 1.0f, 0.0f,
			};

			attribute_position = glGetAttribLocation (shader.program(), "pos");

			glGenBuffers (1, &positionBuffer);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

			mb_threshold = 1.5;

			// put the triangle at the center of the world
			modelToWorld.loadIdentity();
			  
			// put the camera a short distance from the center, looking towards the triangle
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, 5);

			angle = 0.0f;

			// Initialize particles
			// We will make a block of particles with a total width of 1/4 of the screen.
			float w = sim_w/4;
			for(float y=bottom+1; y <= 10000; y+=radius*.5f)
			{
				for(float x=-w; x <= w; x+=radius*.5f)
				{
					if(m_particles.size() > num_of_particles) break;

					Particle p = Particle();
					p.position = Vec2(x, y);
					p.position_old = p.position;// + 0.001f * vec2(rand01(), rand01());
					p.force = Vec2(0,0);
					p.sigma = .2f;
					p.beta = 0.2f;
					m_particles.push_back(p);
				}
			}
		}

		void update()
		{
			//solver.compute_forces(num_of_particles, m_particles);

			for(int i=0; i < num_of_particles; i++)
			{
				// Normal verlet stuff
				m_particles[i].position_old = m_particles[i].position;
				m_particles[i].position += m_particles[i].velocity;

				// Apply the currently accumulated forces
				m_particles[i].position += m_particles[i].force;

				// Restart the forces with gravity only. We'll add the rest later.
				m_particles[i].force = Vec2(0,-gravity);

				// Calculate the velocity for later.
				m_particles[i].velocity = m_particles[i].position - m_particles[i].position_old;

				// If the velocity is really high, we're going to cheat and cap it.
				// This will not damp all motion. It's not physically-based at all. Just
				// a little bit of a hack.
				float max_vel = 2.f;
				float vel_mag = m_particles[i].velocity.len2();
				// If the velocity is greater than the max velocity, then cut it in half.
				if(vel_mag > max_vel*max_vel)
					m_particles[i].velocity = m_particles[i].velocity * .5f;

				// If the particle is outside the bounds of the world, then
				// Make a little spring force to push it back in.
				bound(m_particles[i]);

				// Reset the nessecary items.
				m_particles[i].rho = m_particles[i].rho_near = 0;
				m_particles[i].m_neighbours.clear();				
			}

			calculate_density();
			calculate_pressure();
			calculate_pressure_force();
			calculate_viscosity();
		}

		void bound(Particle p)
		{
			if(p.position.x < -sim_w) p.force.x -= (p.position.x - -sim_w) / 8;
			if(p.position.x >  sim_w) p.force.x -= (p.position.x - sim_w) / 8;
			if(p.position.y < bottom) p.force.y -= (p.position.y - bottom) / 8;
			if(p.position.y > sim_w*2)p.force.y -= (p.position.y - sim_w*2) / 8;
		}


		// DENSITY 
		//
		// Calculate the density by basically making a weighted sum
		// of the distances of neighboring particles within the radius of support (radius)
		void calculate_density()
		{
			for(int i=0; i < m_particles.size(); ++i)
			{
				m_particles[i].rho = m_particles[i].rho_near = 0;

				// We will sum up the 'near' and 'far' densities.
				float d=0, dn=0;

				// Now look at every other particle
				for(int j = i + 1; j < m_particles.size(); ++j)
				{
					// The vector seperating the two particles
					Vec2 rij = m_particles[i].position - m_particles[i].position;

					// Along with the squared distance between
					float rij_len2 = rij.len2();          

					// If they're within the radius of support ...
					if(rij_len2 < radius_sq)
					{
						// Get the actual distance from the squared distance.
						float rij_len = sqrt(rij_len2);

						// And calculated the weighted distance values
						float q = 1 - rij_len / radius;
						float q2 = q*q;
						float q3 = q2*q;

						d += q2;
						dn += q3;

						// Accumulate on the neighbor
						m_particles[i].rho += q2;
						m_particles[i].rho_near += q3;

						// Set up the neighbor list for faster access later.
						Neighbour  neighbour;
						neighbour.i = i; neighbour.j = j;
						neighbour.dist = q; neighbour.dist2 = q2;              
						m_particles[i].m_neighbours.push_back(neighbour);
					}
				}

				m_particles[i].rho        += d;
				m_particles[i].rho_near   += dn;
			}
		}

		// PRESSURE
		//
		// Make the simple pressure calculation from the equation of state.
		void calculate_pressure()
		{

			for(int i=0; i < m_particles.size(); ++i)
			{
				m_particles[i].pressure = k_far * (m_particles[i].rho - rest_density);
				m_particles[i].pressure_near = k_near * m_particles[i].rho_near;
			}
		}

		// PRESSURE FORCE
		//
		// We will force particles in or out from their neighbors
		// based on their difference from the rest density.
		void calculate_pressure_force()
		{

			for(int i=0; i < m_particles.size(); ++i)
			{
				Vec2 dX = Vec2();

				// For each of the neighbors
				int ncount = m_particles[i].m_neighbours.size();
				for(int ni=0; ni < ncount; ++ni)
				{
					Neighbour n = m_particles[i].m_neighbours[ni];
					int j = n.j;     
					float q(n.dist);
					float q2(n.dist2);       

					// The vector from particle i to particle j
					Vec2 rij = m_particles[i].position - m_particles[i].position;

					// calculate the force from the pressures calculated above
					float dm = (m_particles[i].pressure + m_particles[i].pressure) * q +
						(m_particles[i].pressure_near + m_particles[i].pressure_near) * q2;

					// Get the direction of the force
					Vec2 D = rij.normal() * dm;
					dX += D;
					m_particles[i].force += D;
				}

				m_particles[i].force -= dX;
			}

		}

				// PRESSURE FORCE
		//
		// We will force particles in or out from their neighbors
		// based on their difference from the rest density.
		void calculate_viscosity()
		{

			for(int i=0; i < m_particles.size(); ++i)
			{
				// For each of that particles neighbors
				for(int ni=0; ni < m_particles[i].m_neighbours.size(); ++ni)
				{
					Neighbour n = m_particles[i].m_neighbours[ni];

					Vec2 rij = m_particles[n.j].position - m_particles[i].position;
					float l = (rij).len();
					float q = l / radius;

					Vec2 rijn = (rij / l);
					// Get the projection of the velocities onto the vector between them.
					float u = (m_particles[n.i].velocity - m_particles[n.j].velocity) *rijn;
					if(u > 0)
					{
						// Calculate the viscosity impulse between the two particles
						// based on the quadratic function of projected length.
						Vec2 I = (1 - q) * (m_particles[n.j].sigma * u + m_particles[n.j].beta * u*u) * rijn;

						// Apply the impulses on the two particles
						m_particles[n.i].velocity -= I * 0.5;
						m_particles[n.j].velocity += I * 0.5;
					}

				}
			}

		}

		void render()
		{
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);
			//vec4 color(0, 0, 1, 1);
			
			//float color[] = {0, 0, 1, 1,};

			//color_shader_.render(modelToProjection, color);

			//glPointSize(radius*2);
			//glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
			//glVertexAttribPointer(attribute_pos, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), &m_particles[0].position.x); //&vertices[0]
			//glEnableVertexAttribArray(attribute_pos);
      
			//glDrawArrays(GL_POINTS, 0,  m_particles.size() );

			UpdateMetaballs (m_particles);
	
			//float colors[] = {0.5f, 0.5f, 0.5f, 1.0f,};

			//shader.render (modelToProjection, colors);
			shader.render (modelToProjection, colors, mb_positions, mb_threshold, numOfMetaballs);

			glEnableVertexAttribArray (attribute_position);
			glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

			glVertexAttribPointer (attribute_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GLfloat), 0);

			int size;
			glGetBufferParameteriv (GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

			glDrawArrays (GL_TRIANGLES, 0, 6);
		}

		/// this is called to draw the world
		void draw_world(int x, int y, int w, int h)
		{
			glViewport(x, y, w, h);
			  glClearColor(0, 0, 0, 1);
			  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			  glEnable(GL_DEPTH_TEST);

			update();
			render();
		}

		void UpdateMetaballs (std::vector<Particle> p)
		{
			numOfMetaballs = p.size();
			mb_positions = new float[numOfMetaballs * 2];
			//velocities = new float[numOfMetaballs * 3];
			colors = new float[numOfMetaballs * 3];

			for (int i = 0; i < numOfMetaballs; i++)
			{
				printf("Particle %i - X: %f, Y: %f\n", i, p[i].position.x, p[i].position.y);
				mb_positions[i * 2] = p[i].position.x;
				mb_positions[i * 2 + 1] = p[i].position.y;
				//mb_positions[i * 3 + 2] = 0;
				//velocities[0] = -5.0f;
				//velocities[1] = -3.0f;
				//velocities[2] = 0.0f;
				colors[i * 3] = 0.0f;
				colors[i * 3 + 1] = 0.2f;
				colors[i * 3 + 2] = 0.5f;

				//printf("Particle %i - X: %i, Y: %i\n", i, mb_positions[i * 2], mb_positions[i * 2 + 1]);
			}
		}

		float rand01() { return (float)rand() * (1.f / RAND_MAX); }
	};
}

