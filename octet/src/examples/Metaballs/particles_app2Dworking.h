﻿////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012, 2013
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.

namespace octet {

  struct particle {
    vec3 position;
    vec3 velocity;
    float dencity;
  };

  typedef struct sim_param_t {
    char* fname; /* File name */
    int nframes; /* Number of frames */
    int npframe; /* Steps per frame */
    float h; /* Particle size */
    float dt; /* Time step */
    float rho0; /* Reference density */
    float k; /* Bulk modulus */
    float mu; /* Viscosity */
    float g; /* Gravity strength */
  } sim_param_t;
  //The array x has length 2n, with
  //x[2*i+0] and x[2*i+1] representing the x and y coordinates of the particle
  //positions. The layout for v, vh, and a is similar, while rho only has one entry
  //per particle.
  typedef struct sim_state_t {
    int n; /* Number of particles */
    float mass; /* Particle mass */
    float* rho; /* Densities */
    float* x; /* Positions */
    float* vh; /* Velocities (half step) */
    float* v; /* Velocities (full step) */
    float* a; /* Acceleration */
  } sim_state_t;

  sim_state_t* alloc_state(int n, sim_state_t *p) {
    int size = 2*n;  // I think 2*n not n*n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //static sim_state_t *p = new sim_state_t();
    p->n = n;
    p->rho = (float *) malloc ( n*sizeof(float) ); // rho is density and has one entry per particle
    p->x = (float *) malloc ( size*sizeof(float) );
    p->vh = (float *) malloc ( size*sizeof(float) );
    p->v = (float *) malloc ( size*sizeof(float) );
    p->a = (float *) malloc ( size*sizeof(float) );
    return p;
  }
  void free_state(sim_state_t* s);

  static void default_params(sim_param_t* params)
  {
    params->fname = "run.out";
    params->nframes = 400;
    params->npframe = 100;
    params->dt = (float)0.001;//1e-4;
    params->h = (float)0.03;//5e-2;
    params->rho0 = 1000; // reference density
    params->k = 1e3; // bulk modulus
    params->mu = 10.1f; // viscocity
    params->g = 9.8f;
  }

  class particles_app : public app {
    mat4t modelToWorld;
    mat4t cameraToWorld;
    //color_shader shader;
	metaball_shader shader;
	  float angle;
    sim_param_t params;
    sim_state_t* state;
	int* mb_positions;
	float* colors;
	int numOfMetaballs;
	int mb_threshold;
	GLuint positionBuffer, attribute_position;
	//Metaballs metaballs;

    //dynarray<float> vertices;
  public:

    // this is called when we construct the class
    particles_app(int argc, char **argv) : app(argc, argv) {
    }

	~particles_app ()
	{
		delete[] mb_positions;
		delete[] colors;
	}

    // this is called once OpenGL is initialized
    void app_init() {
      // initialize the shader
      //color_shader_.init();
		shader.init();
	  //metaballs.InitMetaballs();

      // put the triangle at the center of the world
      modelToWorld.loadIdentity();

      // put the camera a short distance from the center, looking towards the triangle
      cameraToWorld.loadIdentity();
      cameraToWorld.translate(0, 0, 1);

	    angle = 0.0f;

      state = init_particles(&params);
      int nframes = params.nframes;
      int npframe = params.npframe;
      float dt = params.dt;
      int n = state->n;
      compute_accel(state, &params);
      leapfrog_start(state, dt);
      check_state(state);

	  attribute_position = glGetAttribLocation (shader.program(), "pos");
		//int i = 0;

		float vertices[] = {
			-1.0f, -1.0f, 0.0f,
			 1.0f, -1.0f, 0.0f,
			 -1.0f,  1.0f, 0.0f,

			1.0f, -1.0f, 0.0f,
			-1.0f, 1.0f, 0.0f,
			1.0f, 1.0f, 0.0f,
		};

		glGenBuffers (1, &positionBuffer);
		glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

		glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

		mb_threshold = 1.5;
      //for (int frame = 1; frame < params.nframes; ++frame) {
      //  for (int i = 0; i < params.npframe; ++i) {
      //    compute_accel(state, &params);
      //    leapfrog_step(state, params.dt);
      //    check_state(state);
      //  }
      //}

    }

    void compute_density(sim_state_t* s, sim_param_t* params)
    {
      int n = s->n;
      float* rho = s->rho;
      const float* x = s->x;
      float h = params->h;
      float h2 = h*h;
      float h8 = ( h2*h2 )*( h2*h2 );
      float C = 4 * s->mass / 3.14f / h8;  // 4m/(π*h^8)
      memset(rho, 0, n*sizeof(float));
      for (int i = 0; i < n; ++i) {
        rho[i] += 4 * s->mass / 3.14f / h2;
        for (int j = i+1; j < n; ++j) {
          float dx = x[2*i+0]-x[2*j+0];
          float dy = x[2*i+1]-x[2*j+1];
          // x*x + y*y = r*r
          // next two lines check about the distance in a circle, if another particle is inside its radius then take it into consideration
          float r2 = dx*dx + dy*dy;
          float z = h2-r2;
          if (z > 0) {
            float rho_ij = C*z*z*z;
            rho[i] += rho_ij;
            rho[j] += rho_ij;
          }
        }
      }
    }

void compute_accel(sim_state_t* state, sim_param_t* params)
{
  // Unpack basic parameters
  const float h = params->h;
  const float rho0 = params->rho0;
  const float k = params->k;
  const float mu = params->mu;
  const float g = params->g;
  const float mass = state->mass;
  const float h2 = h*h;
  // Unpack system state
  const float* rho = state->rho;
  const float* x = state->x;
  const float* v = state->v;
  float* a = state->a;
  int n = state->n;
  // Compute density and color
  compute_density(state, params);
  // Start with gravity and surface forces
  for (int i = 0; i < n; ++i) {
    a[2*i+0] = 0;
    a[2*i+1] = -g;
  }
  // Constants for interaction term
  float C0 = mass / 3.14f / ( (h2)*(h2) );
  float Cp = 15*k;
  float Cv = -40*mu;
  // Now compute interaction forces
  for (int i = 0; i < n; ++i) {
    const float rhoi = rho[i];
    for (int j = i+1; j < n; ++j) {
      float dx = x[2*i+0]-x[2*j+0];
      float dy = x[2*i+1]-x[2*j+1];
      float r2 = dx*dx + dy*dy;
      // the particles that are not inside the radius contribute to acceleration
      if (r2 < h2) {
        const float rhoj = rho[j];
        float q = sqrt(r2)/h;
        float u = 1-q;
        float w0 = C0 * u/rhoi/rhoj;
        float wp = w0 * Cp * (rhoi+rhoj-2*rho0) * u/q;
        float wv = w0 * Cv;
        float dvx = v[2*i+0]-v[2*j+0];
        float dvy = v[2*i+1]-v[2*j+1];
        a[2*i+0] += (wp*dx + wv*dvx);
        a[2*i+1] += (wp*dy + wv*dvy);
        a[2*j+0] -= (wp*dx + wv*dvx);
        a[2*j+1] -= (wp*dy + wv*dvy);
       }
      }
    }
}
//Leapfrog integration is equivalent to updating positions x(t) and velocities v(t) at interleaved time points,
//staggered in such a way that they 'leapfrog' over each other.
// the position is updated at integer time steps and the velocity is updated at integer-plus-a-half time steps.
//The leapfrog time integration algorithm is named because the velocities are
//updated on half steps and the positions on integer steps; hence, the two leap
//over each other.
// we compute the v^(i+1/2) stored in vh and we compute an approximation of v^(i+1) (stored in v) 
void leapfrog_step(sim_state_t* s, double dt)
{
  const float* a = s->a;
  float* vh = s->vh;
  float* v = s->v;
  float* x = s->x;
  int n = s->n;
  for (int i = 0; i < 2*n; ++i) { vh[i] += a[i] * dt; }
  for (int i = 0; i < 2*n; ++i) { v[i] = vh[i] + a[i] * dt / 2; }
  for (int i = 0; i < 2*n; ++i) { x[i] += vh[i] * (float)dt; }
  reflect_bc(s); // reflect the particles
}
// At the first step, the leapfrog iteration only has the initial velocities v0, so we need to do something special
void leapfrog_start(sim_state_t* s, double dt)
{
  const float* a = s->a;
  float* vh = s->vh;
  float* v = s->v;
  float* x = s->x;
  int n = s->n;
  for (int i = 0; i < 2*n; ++i) { vh[i] = v[i] + a[i] * dt / 2; }
  for (int i = 0; i < 2*n; ++i) { v[i] += a[i] * dt; }
  for (int i = 0; i < 2*n; ++i) { x[i] += vh[i] * dt; }
  reflect_bc(s);
}
// which == 0 vertical barrier
// which == 1 horrizontal barrier
static void damp_reflect(int which, float barrier, float* x, float* v, float* vh)
{
  // Coefficient of resitiution
  const float DAMP = 0.75;
  // Ignore degenerate cases
  if (v[which] == 0)
  return;
  // Scale back the distance traveled based on time from collision
  float tbounce = (x[which]-barrier)/v[which];
  x[0] -= v[0]*(1-DAMP)*tbounce;
  x[1] -= v[1]*(1-DAMP)*tbounce;
  // Reflect the position and velocity
  x[which] = 2*barrier-x[which];
  v[which] = -v[which];
  vh[which] = -vh[which];
  // Damp the velocities
  v[0] *= DAMP; vh[0] *= DAMP;
  v[1] *= DAMP; vh[1] *= DAMP;
}
// For each particle, we need to check for reflections on each of the four walls of the computational domain.
static void reflect_bc(sim_state_t* s)
{
  // Boundaries of the computational domain
  const float XMIN = 0.0;
  const float XMAX = 1.0;
  const float YMIN = 0.0;
  const float YMAX = 1.0;
  float* vh = s->vh;
  float* v = s->v;
  float* x = s->x;
  int n = s->n;
  for (int i = 0; i < n; ++i, x += 2, v += 2, vh += 2) {
    // x[0] is the position of each particle at x axis
    if (x[0] < XMIN) damp_reflect(0, XMIN, x, v, vh);
    if (x[0] > XMAX) damp_reflect(0, XMAX, x, v, vh);
    // x[1] is the position of each particle at y axis
    if (x[1] < YMIN) damp_reflect(1, YMIN, x, v, vh);
    if (x[1] > YMAX) damp_reflect(1, YMAX, x, v, vh);
  }
}

typedef int (*domain_fun_t)(float, float);
domain_fun_t functPointer;
int box_indicator(float x, float y)
{
  return (x < 0.5f) && (y < 0.5f);
  //return (x < 3.5f) && (y < 3.5f);
}
int circ_indicator(float x, float y)
{
  float dx = (x-0.5);
  float dy = (y-0.3);
  float r2 = dx*dx + dy*dy;
  return (r2 < 0.25*0.25);
}
// The place particle routine determines the initial particle placement, but not the desired mass.
sim_state_t* place_particles(sim_param_t* param)  //, domain_fun_t indicatef
{
  float h = param->h;
  float hh = h/1.3f; // h/1.3f; // this determine the number of particles
  // Count mesh points that fall in indicated region.
  int count = 0;
  for (float x = 0; x < 1; x += hh) {   // x < 1 // I think it needs to have {}!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (float y = 0; y < 1; y += hh)   // y < 1
      count += box_indicator(x,y);//indicatef(x,y);
  }
  // Populate the particle data structure
  sim_state_t* s = new sim_state_t();
  s = alloc_state(count, s);
  int p = 0;
  for (float x = 0; x < 1; x += hh) {
    for (float y = 0; y < 1; y += hh) {
      if (box_indicator(x,y)) {   //indicatef(x,y)
        s->x[2*p+0] = x;
        s->x[2*p+1] = y;
        s->v[2*p+0] = 0;
        s->v[2*p+1] = 0;
        ++p;
      }
    }
  }
  return s;
}

void normalize_mass(sim_state_t* s, sim_param_t* param)
{
  s->mass = 1;
  compute_density(s, param);
  float rho0 = param->rho0;
  float rho2s = 0;
  float rhos = 0;
  for (int i = 0; i < s->n; ++i) {
    rho2s += (s->rho[i])*(s->rho[i]);
    rhos += s->rho[i];
  }
  s->mass *= ( rho0*rhos / rho2s );
}

sim_state_t* init_particles(sim_param_t* param)
{
  default_params(param);
  sim_state_t* s = place_particles(param); //, box_indicator
  normalize_mass(s, param);
  return s;
}

void check_state(sim_state_t* s)
{
  //for (int i = 0; i < s->n; ++i) {
  //  float xi = s->x[2*i+0];
  //  float yi = s->x[2*i+1];
  //  assert( xi >= 0 || xi <= 1 );
  //  assert( yi >= 0 || yi <= 1 );
  //}
}
    // this is called to draw the world
    void draw_world(int x, int y, int w, int h) {
		int vx, vy;
		get_viewport_size (vx, vy);

		
      glViewport(x, y, w, h);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);
      //vec4 color(0, 0, 1, 1);
      //color_shader_.render(modelToProjection, color.get());

      compute_accel(state, &params);
      leapfrog_step(state, params.dt);
      //check_state(state);
     
	  UpdateMetaballs (state->x, state->n, vx, vy);
	
	  //float colors[] = {0.5f, 0.5f, 0.5f, 1.0f,};

	  //shader.render (modelToProjection, colors);
	  shader.render (modelToProjection, colors, mb_positions, mb_threshold, numOfMetaballs);

		glEnableVertexAttribArray (attribute_position);
		glBindBuffer (GL_ARRAY_BUFFER, positionBuffer);

		glVertexAttribPointer (attribute_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof (GLfloat), 0);

		int size;
		glGetBufferParameteriv (GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

		glDrawArrays (GL_TRIANGLES, 0, 6);

	  //metaballs.UpdateMetaballs(state->x, state->n, vx, vy);
	  //metaballs.renderMetaballs();

      //glPointSize(1.5f);
      //glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)vertices );
      //glVertexAttribPointer(attribute_pos, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)state->x); //&vertices[0]
      //glEnableVertexAttribArray(attribute_pos);
      
      //glDrawArrays(GL_POINTS, 0,  state->n );

	  
	  if (is_key_down(key_left)) {
		  cameraToWorld.rotateX(-angle);
		  cameraToWorld.rotateY(10.0f);
		  cameraToWorld.rotateX(angle);
      }
	  else if (is_key_down(key_right)) {
		  cameraToWorld.rotateX(-angle);
		  cameraToWorld.rotateY(-10.0f);
		  cameraToWorld.rotateX(angle);
      }
	  else if (is_key_down(key_up)) {
		  angle += 10.0f;
		  cameraToWorld.rotateX(10.0f);
      }
	  else if (is_key_down(key_down)) {
		  angle -= 10.0f;
		  cameraToWorld.rotateX(-10.0f);
      }
	  else if (is_key_down('W'))
		  cameraToWorld.translate(0.0f, 0.0f, -0.1f);
	  else if (is_key_down('S'))
		  cameraToWorld.translate(0.0f, 0.0f, 0.1f);
	  else if (is_key_down('A'))
		  cameraToWorld.translate(-1.0f, 0.0f, 0.1f);
	  else if (is_key_down('D'))
		  cameraToWorld.translate(1.0f, 0.0f, 0.1f);
		  
    }

	void UpdateMetaballs (float* pos, const int &size, const int &vx, const int &vy)
	{
		numOfMetaballs = 100;
		mb_positions = new int[numOfMetaballs * 2];
		//velocities = new float[numOfMetaballs * 3];
		colors = new float[numOfMetaballs * 3];

		for (int i = 0; i < numOfMetaballs; i++)
		{
			//printf("Particle %i - X: %f, Y: %f\n", i, pos[i * 2], pos[i * 2 + 1]);
			mb_positions[i * 2] = pos[i * 2] * vx;
			mb_positions[i * 2 + 1] = pos[i * 2 + 1] * vy;
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
  };
}
