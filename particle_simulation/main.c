#include <raylib.h>

typedef struct {
  float x;
  float y;
} Vec2;

struct Particle {
  Vec2 position;
  Vec2 previous_position;
  Vec2 acceleration;
  float radius;
};

struct Particle create_particle(float dt) {
  struct Particle particle = {.position = {.x = 0.0f, .y = 10.0f},
                              .acceleration = {.x = 0.0f, .y = -9.81f},
                              .radius = 0.25f};

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

void collide_with_floor(struct Particle *particle, float dt) {
  const float floor_y = 0.0f;
  const float restitution = 0.8f;
  const float resting_speed = 0.5f;
  float minimum_y = floor_y + particle->radius;

  if (particle->position.y < minimum_y) {
    // Estimate vertical velocity before correcting the position.
    float velocity_y =
        (particle->position.y - particle->previous_position.y) / dt;

    particle->position.y = minimum_y;

    if (velocity_y < 0.0f) {
      velocity_y = -velocity_y * restitution;
    }

    // Stop tiny bounces so the particle can settle.
    if (velocity_y < resting_speed) {
      velocity_y = 0.0f;
    }

    // Encode the new motion in the previous position.
    particle->previous_position.y = particle->position.y - velocity_y * dt;
  }
}

void collide_with_walls(struct Particle *particle, float dt, float left,
                        float right) {
  const float restitution = 0.8f;
  float minimum_x = left + particle->radius;
  float maximum_x = right - particle->radius;
  float velocity_x =
      (particle->position.x - particle->previous_position.x) / dt;

  if (particle->position.x < minimum_x) {
    particle->position.x = minimum_x;

    if (velocity_x < 0.0f) {
      velocity_x = -velocity_x * restitution;
    }
  } else if (particle->position.x > maximum_x) {
    particle->position.x = maximum_x;

    if (velocity_x > 0.0f) {
      velocity_x = -velocity_x * restitution;
    }
  } else {
    return;
  }

  particle->previous_position.x = particle->position.x - velocity_x * dt;
}

int main(void) {
  const float dt = 1.0f / 60.0f;
  const float pixels_per_meter = 40.0f;
  const float origin_x = 80.0f;
  const float origin_y = 550.0f;
  const float left_wall = (20.0f - origin_x) / pixels_per_meter;
  const float right_wall = (780.0f - origin_x) / pixels_per_meter;

  float accumulator = 0.0f;
  struct Particle particle = create_particle(dt);

  InitWindow(800, 600, "Particle Simulation");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    float frame_time = GetFrameTime();

    if (frame_time > 0.25f) {
      frame_time = 0.25f;
    }

    if (IsKeyPressed(KEY_R)) {
      particle = create_particle(dt);
      accumulator = 0.0f;
    } else {
      accumulator += frame_time;
    }

    while (accumulator >= dt) {
      update_particle(&particle, dt);
      collide_with_floor(&particle, dt);
      collide_with_walls(&particle, dt, left_wall, right_wall);
      accumulator -= dt;
    }

    float screen_x = origin_x + particle.position.x * pixels_per_meter;
    float screen_y = origin_y - particle.position.y * pixels_per_meter;

    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawText("R: restart | Esc: close", 20, 20, 20, DARKGRAY);
    DrawLine(0, (int)origin_y, 800, (int)origin_y, DARKGRAY);
    DrawLine(20, 50, 20, (int)origin_y, DARKGRAY);
    DrawLine(780, 50, 780, (int)origin_y, DARKGRAY);
    DrawCircle((int)screen_x, (int)screen_y, particle.radius * pixels_per_meter,
               BLUE);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
