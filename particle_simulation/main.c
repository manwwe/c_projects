#include <stdio.h>

typedef struct {
  float x;
  float y;
} Vec2;

struct Particle {
  Vec2 position;
  Vec2 previous_position;
  Vec2 acceleration;
};

void update_particle(struct Particle *particle, float dt) {
  Vec2 current = particle->position;

  particle->position.x += (current.x - particle->previous_position.x) +
                          particle->acceleration.x * dt * dt;

  particle->position.y += (current.y - particle->previous_position.y) +
                          particle->acceleration.y * dt * dt;

  particle->previous_position = current;
}

int main(void) {
  struct Particle particle = {.position = {.x = 0.0f, .y = 10.0f},
                              .previous_position = {.x = 0.0f, .y = 10.0f},
                              .acceleration = {.x = 0.0f, .y = -9.81f}};

  float dt = 0.1f;

  // Initialize the previous position for a particle starting at rest.
  particle.previous_position.x =
      particle.position.x + 0.5f * particle.acceleration.x * dt * dt;
  particle.previous_position.y =
      particle.position.y + 0.5f * particle.acceleration.y * dt * dt;

  for (int step = 0; step < 10; step++) {
    update_particle(&particle, dt);

    printf("Position: (%f, %f)\n", particle.position.x, particle.position.y);
  }

  return 0;
}
