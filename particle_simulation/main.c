#include <raylib.h>

typedef struct {
  float x;
  float y;
} Vec2;

struct Particle {
  Vec2 position;
  Vec2 previous_position;
  Vec2 acceleration;
};

struct Particle create_particle(float dt) {
  struct Particle particle = {.position = {.x = 0.0f, .y = 10.0f},
                              .acceleration = {.x = 0.0f, .y = -9.81f}};

  Vec2 velocity = {.x = 2.0f, .y = 0.0f};

  particle.previous_position.x = particle.position.x - velocity.x * dt +
                                 0.5f * particle.acceleration.x * dt * dt;

  particle.previous_position.y = particle.position.y - velocity.y * dt +
                                 0.5f * particle.acceleration.y * dt * dt;

  return particle;
}

void update_particle(struct Particle *particle, float dt) {
  Vec2 current = particle->position;

  particle->position.x += (current.x - particle->previous_position.x) +
                          particle->acceleration.x * dt * dt;

  particle->position.y += (current.y - particle->previous_position.y) +
                          particle->acceleration.y * dt * dt;

  particle->previous_position = current;
}

int main(void) {
  const float dt = 1.0f / 60.0f;
  const float pixels_per_meter = 40.0f;
  float accumulator = 0.0f;

  struct Particle particle = create_particle(dt);

  InitWindow(800, 600, "Particle Simulation");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    float frame_time = GetFrameTime();

    // Avoid a large catch-up after a pause or window drag.
    if (frame_time > 0.25f) {
      frame_time = 0.25f;
    }

    if (IsKeyPressed(KEY_R)) {
      particle = create_particle(dt);
      accumulator = 0.0f;
    } else {
      accumulator += frame_time;
    }

    // Physics always advances by the same time step.
    while (accumulator >= dt) {
      update_particle(&particle, dt);
      accumulator -= dt;
    }

    float screen_x = 80.0f + particle.position.x * pixels_per_meter;
    float screen_y = 550.0f - particle.position.y * pixels_per_meter;

    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawText("R: restart | Esc: close", 20, 20, 20, DARKGRAY);
    DrawCircle((int)screen_x, (int)screen_y, 10.0f, BLUE);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
