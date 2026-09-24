#include <raylib.h>
#include <raymath.h>
#include <math.h>

#define INITIAL_PARTICLE_COUNT 12
#define MAX_PARTICLES 350
#define CONTAINER_RADIUS 6.0f
#define PARTICLE_RADIUS 0.30f
#define CONTACT_PASSES 8
#define EMISSION_INTERVAL 0.04f
#define RESTITUTION 0.25f
#define BOUNCE_SPEED 0.5f
#define PHYSICS_DT (1.0f / 120.0f)
#define GRAVITY_Y (-9.81f)
#define BOUNDARY_ROTATION_SPEED 0.075f

struct Particle {
  Vector3 position;
  Vector3 previous_position;
  float radius;
};

struct Particle create_particle(Vector3 position, Vector3 velocity) {
  return (struct Particle){
      .position = position,
      .previous_position = Vector3Add(
          Vector3Subtract(position, Vector3Scale(velocity, PHYSICS_DT)),
          (Vector3){0, 0.5f * GRAVITY_Y * PHYSICS_DT * PHYSICS_DT, 0}),
      .radius = PARTICLE_RADIUS};
}

void reset_particles(struct Particle particles[], int *count) {
  *count = INITIAL_PARTICLE_COUNT;
  for (int i = 0; i < *count; i++) {
    float angle = i * 2.399963f;
    float y = -2.5f + 5.0f * i / (*count - 1);
    Vector3 position = {2.8f * cosf(angle), y, 2.8f * sinf(angle)};
    Vector3 velocity = {1.4f * sinf(angle + 0.7f),
                        1.1f * cosf(angle * 1.3f),
                        1.4f * cosf(angle + 0.7f)};
    particles[i] = create_particle(position, velocity);
  }
}

void update_particle(struct Particle *particle) {
  Vector3 current = particle->position;
  Vector3 motion = Vector3Subtract(current, particle->previous_position);
  particle->position = Vector3Add(current, motion);
  particle->position.y += GRAVITY_Y * PHYSICS_DT * PHYSICS_DT;
  particle->previous_position = current;
}

void collide_particles(struct Particle *a, struct Particle *b) {
  Vector3 delta = Vector3Subtract(b->position, a->position);
  float radius = a->radius + b->radius;
  // Most pairs are far apart; reject them before the more expensive contact work.
  if (fabsf(delta.x) > radius || fabsf(delta.y) > radius || fabsf(delta.z) > radius)
    return;
  float distance_squared = Vector3LengthSqr(delta);
  if (distance_squared > radius * radius) return;

  Vector3 motion_a = Vector3Subtract(a->position, a->previous_position);
  Vector3 motion_b = Vector3Subtract(b->position, b->previous_position);
  float distance = sqrtf(distance_squared);
  Vector3 normal = distance > 0.0f ? Vector3Scale(delta, 1.0f / distance)
                                  : (Vector3){1, 0, 0};
  Vector3 correction = Vector3Scale(normal, 0.5f * (radius - distance));
  a->position = Vector3Subtract(a->position, correction);
  b->position = Vector3Add(b->position, correction);

  float closing_motion = Vector3DotProduct(Vector3Subtract(motion_b, motion_a), normal);
  if (closing_motion < 0.0f) {
    // Gentle contacts do not bounce, allowing the pile to settle.
    float restitution = -closing_motion > BOUNCE_SPEED * PHYSICS_DT ? RESTITUTION : 0.0f;
    Vector3 impulse = Vector3Scale(normal, -0.5f * (1.0f + restitution) * closing_motion);
    motion_a = Vector3Subtract(motion_a, impulse);
    motion_b = Vector3Add(motion_b, impulse);
    // Contact friction reduces sliding without changing the pair's total momentum.
    Vector3 relative = Vector3Subtract(motion_b, motion_a);
    Vector3 tangent = Vector3Subtract(relative, Vector3Scale(normal, Vector3DotProduct(relative, normal)));
    float tangent_length = Vector3Length(tangent);
    if (tangent_length > 0.000001f) {
      float friction = fminf(0.5f * tangent_length, 0.3f * Vector3Length(impulse));
      Vector3 friction_motion = Vector3Scale(tangent, friction / tangent_length);
      motion_a = Vector3Add(motion_a, friction_motion);
      motion_b = Vector3Subtract(motion_b, friction_motion);
    }
  }
  a->previous_position = Vector3Subtract(a->position, motion_a);
  b->previous_position = Vector3Subtract(b->position, motion_b);
}

void collide_with_container(struct Particle *particle) {
  float limit = CONTAINER_RADIUS - particle->radius;
  float distance = Vector3Length(particle->position);
  if (distance <= limit) return;

  Vector3 motion = Vector3Subtract(particle->position, particle->previous_position);
  Vector3 normal = Vector3Scale(particle->position, 1.0f / distance);
  particle->position = Vector3Scale(normal, limit);
  float outward_motion = Vector3DotProduct(motion, normal);
  if (outward_motion > 0.0f) {
    float restitution = outward_motion > BOUNCE_SPEED * PHYSICS_DT ? RESTITUTION : 0.0f;
    motion = Vector3Subtract(motion, Vector3Scale(normal, (1.0f + restitution) * outward_motion));
    Vector3 tangent = Vector3Subtract(motion, Vector3Scale(normal, Vector3DotProduct(motion, normal)));
    float tangent_length = Vector3Length(tangent);
    if (tangent_length > 0.000001f) {
      float friction = fminf(tangent_length, 0.3f * (1.0f + restitution) * outward_motion);
      motion = Vector3Subtract(motion, Vector3Scale(tangent, friction / tangent_length));
    }
  }
  particle->previous_position = Vector3Subtract(particle->position, motion);
}

void step_simulation(struct Particle particles[], int count) {
  for (int i = 0; i < count; i++) update_particle(&particles[i]);
  // bunter: discrete contacts can miss very fast crossings; use swept tests if speeds increase.
  for (int pass = 0; pass < CONTACT_PASSES; pass++) {
    for (int i = 0; i < count; i++) {
      for (int j = i + 1; j < count; j++) collide_particles(&particles[i], &particles[j]);
    }
    for (int i = 0; i < count; i++) collide_with_container(&particles[i]);
  }
}

bool spawn_particle(struct Particle particles[], int *count, Vector3 position,
                    Vector3 velocity) {
  if (*count >= MAX_PARTICLES ||
      Vector3Length(position) > CONTAINER_RADIUS - PARTICLE_RADIUS) return false;
  for (int i = 0; i < *count; i++) {
    float separation = particles[i].radius + PARTICLE_RADIUS;
    if (Vector3DistanceSqr(position, particles[i].position) < separation * separation)
      return false;
  }
  particles[(*count)++] = create_particle(position, velocity);
  return true;
}

void emit_particle(struct Particle particles[], int *count, int *sequence) {
  if (*count >= MAX_PARTICLES) return;
  // Spread the rain across the upper part of the sphere, avoiding occupied spots.
  for (int attempt = 0; attempt < 16; attempt++) {
    int index = (*sequence)++;
    float angle = index * 2.399963f;
    float radius = 0.7f + 2.2f * ((index * 37) % 101) / 100.0f;
    Vector3 position = {radius * cosf(angle), 4.2f, radius * sinf(angle)};
    if (spawn_particle(particles, count, position, (Vector3){0, -0.3f, 0})) return;
  }
}

void draw_container(float rotation) {
  // Latitude rings of small dots make the spherical boundary visible without a solid shell.
  for (int ring = 1; ring < 24; ring++) {
    float latitude = PI * ring / 24.0f;
    float ring_radius = CONTAINER_RADIUS * sinf(latitude);
    int dots = (int)(80.0f * sinf(latitude));
    for (int dot = 0; dot < dots; dot++) {
      float angle = 2.0f * PI * dot / dots + rotation;
      Vector3 position = {ring_radius * cosf(angle), CONTAINER_RADIUS * cosf(latitude),
                          ring_radius * sinf(angle)};
      DrawPoint3D(position, (Color){100, 101, 108, 255});
    }
  }
}

// Simple directional lighting gives the purple spheres depth, like the reference.
static const char *vertex_shader =
    "#version 330\n"
    "in vec3 vertexPosition; in vec3 vertexNormal;\n"
    "uniform mat4 mvp; uniform mat4 matNormal;\n"
    "out vec3 normal;\n"
    "void main() { normal = normalize(mat3(matNormal)*vertexNormal);"
    "gl_Position = mvp*vec4(vertexPosition, 1.0); }\n";
static const char *fragment_shader =
    "#version 330\n"
    "in vec3 normal; uniform vec4 colDiffuse; out vec4 finalColor;\n"
    "void main() { float light = max(dot(normalize(normal),"
    "normalize(vec3(-0.5, 0.8, 0.6))), 0.0);"
    "finalColor = vec4(colDiffuse.rgb*(0.22 + 0.78*light), colDiffuse.a); }\n";

int main(void) {
  struct Particle particles[MAX_PARTICLES];
  int particle_count;
  reset_particles(particles, &particle_count);
  float accumulator = 0.0f;
  float emission_time = 0.0f;
  int emission_sequence = 0;
  float boundary_rotation = 0.0f;
  bool paused = false;
  float yaw = 0.4f, pitch = 0.15f, camera_distance = 19.0f;
  const char *message = "";
  float message_time = 0.0f;

  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(1100, 800, "Particle Simulation - 3D");
  SetTargetFPS(60);

  Camera3D camera = {.target = {0, 0, 0}, .up = {0, 1, 0},
                     .fovy = 45.0f, .projection = CAMERA_PERSPECTIVE};
  Shader lighting = LoadShaderFromMemory(vertex_shader, fragment_shader);
  Model sphere = LoadModelFromMesh(GenMeshSphere(1.0f, 16, 24));
  sphere.materials[0].shader = lighting;

  while (!WindowShouldClose()) {
    float elapsed = fminf(GetFrameTime(), 0.25f);
    message_time -= elapsed;
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 delta = GetMouseDelta();
      yaw -= delta.x * 0.006f;
      pitch = Clamp(pitch + delta.y * 0.006f, -1.4f, 1.4f);
    }
    camera_distance = Clamp(camera_distance - GetMouseWheelMove(), 15.5f, 32.0f);
    camera.position = (Vector3){camera_distance * cosf(pitch) * sinf(yaw),
                                camera_distance * sinf(pitch),
                                camera_distance * cosf(pitch) * cosf(yaw)};

    if (IsKeyPressed(KEY_SPACE)) {
      paused = !paused;
      accumulator = 0.0f;
    }
    if (IsKeyPressed(KEY_R)) {
      reset_particles(particles, &particle_count);
      accumulator = 0.0f;
      boundary_rotation = 0.0f;
      emission_time = 0.0f;
      emission_sequence = 0;
      paused = false;
      message_time = 0.0f;
    } else {
      // A click selects a point on the plane through the center facing the camera.
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && GetMouseY() > 70 &&
          GetMouseY() < GetScreenHeight() - 45) {
        Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);
        Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        float denominator = Vector3DotProduct(ray.direction, forward);
        if (fabsf(denominator) > 0.0001f) {
          float t = Vector3DotProduct(Vector3Subtract(camera.target, ray.position), forward) /
                    denominator;
          Vector3 position = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
          if (t > 0 && spawn_particle(particles, &particle_count, position,
                                      Vector3Scale(ray.direction, 2.0f))) {
            message = "Particle added";
          } else {
            message = particle_count == MAX_PARTICLES ? "Particle limit reached" :
                      "Click an empty spot inside the sphere";
          }
          message_time = 2.0f;
        }
      }
      if (!paused) accumulator += elapsed;
    }
    while (accumulator >= PHYSICS_DT) {
      emission_time += PHYSICS_DT;
      while (emission_time >= EMISSION_INTERVAL) {
        emit_particle(particles, &particle_count, &emission_sequence);
        emission_time -= EMISSION_INTERVAL;
      }
      step_simulation(particles, particle_count);
      // Visual rotation only: a smooth sphere has the same collision surface at every angle.
      boundary_rotation = fmodf(boundary_rotation + BOUNDARY_ROTATION_SPEED * PHYSICS_DT,
                                2.0f * PI);
      accumulator -= PHYSICS_DT;
    }

    BeginDrawing();
    ClearBackground((Color){18, 18, 20, 255});
    BeginMode3D(camera);
    draw_container(boundary_rotation);
    for (int i = 0; i < particle_count; i++) {
      DrawModel(sphere, particles[i].position, particles[i].radius,
                (Color){166, 65, 211, 255});
    }
    EndMode3D();
    DrawText(TextFormat("FPS: %d  |  Particles: %d/%d%s", GetFPS(), particle_count,
                        MAX_PARTICLES, paused ? "  |  PAUSED" : ""),
             24, 22, 18, (Color){175, 175, 183, 255});
    if (message_time > 0) DrawText(message, 24, 48, 16, (Color){192, 144, 218, 255});
    DrawText("Click: add  |  Right-drag: orbit  |  Scroll: zoom  |  Space: pause  |  R: reset",
             24, GetScreenHeight() - 30, 16, (Color){150, 150, 160, 255});
    EndDrawing();
  }
  UnloadModel(sphere);
  UnloadShader(lighting);
  CloseWindow();
  return 0;
}
