

#include "backward/backward.hpp"

namespace backward {
backward::SignalHandling sh;
} // namespace backward
#include "std_include.h"
//

#include "box2d/box2d.h"
#include "rl.h"

#define AFTER_HOURS_INCLUDE_DERIVED_CHILDREN
#define AFTER_HOURS_ENTITY_HELPER
#define AFTER_HOURS_ENTITY_QUERY
#define AFTER_HOURS_SYSTEM
#include "afterhours/ah.h"
#define AFTER_HOURS_USE_RAYLIB
#include "afterhours/src/developer.h"
#include "afterhours/src/plugins/input_system.h"
#include "afterhours/src/plugins/window_manager.h"
#include <cassert>

//
using namespace afterhours;

typedef raylib::Vector2 vec2;
typedef raylib::Vector3 vec3;
typedef raylib::Vector4 vec4;

constexpr float distance_sq(const vec2 a, const vec2 b) {
  return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

namespace myutil {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }
} // namespace myutil

struct RenderFPS : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderFPS() {}
  virtual void for_each_with(
      const Entity &,
      const window_manager::ProvidesCurrentResolution &pCurrentResolution,
      float) const override {
    raylib::DrawFPS((int)(pCurrentResolution.width() - 80), 0);
  }
};

enum class InputAction { None, Accel, Left, Right, Brake };

using afterhours::input;

auto get_mapping() {
  std::map<InputAction, input::ValidInputs> mapping;
  mapping[InputAction::Accel] = {
      raylib::KEY_UP,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = -1,
      },
  };
  mapping[InputAction::Brake] = {
      raylib::KEY_DOWN,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_RIGHT_Y,
          .dir = 1,
      },
  };

  mapping[InputAction::Left] = {
      raylib::KEY_LEFT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = -1,
      },
  };

  mapping[InputAction::Right] = {
      raylib::KEY_RIGHT,
      input::GamepadAxisWithDir{
          .axis = raylib::GAMEPAD_AXIS_LEFT_X,
          .dir = 1,
      },
  };

  return mapping;
}

float vec_mag(vec2 v) { return sqrt(v.x * v.x + v.y * v.y); }
vec2 vec_norm(vec2 v) {
  float mag = vec_mag(v);
  if (mag == 0)
    return v;
  return vec2{
      v.x / mag,
      v.y / mag,
  };
}

struct HasBody : BaseComponent {
  b2BodyId id;

  HasBody(b2BodyId id_) : id(id_) {}
};

float as_deg(float angle) { return static_cast<float>(angle * 180.f / M_PI); }
float as_rad(float angle) {
  return static_cast<float>(angle * (M_PI / 180.0f));
}

struct Transform : BaseComponent {
  vec2 position;
  vec2 velocity;
  vec2 size;
  float angle = 0;

  vec2 pos() const { return position; }
  void update(vec2 &v) { position = v; }
  Transform(vec2 pos, vec2 sz) : position(pos), size(sz) {}
  raylib::Rectangle rect() const {
    return raylib::Rectangle{position.x, position.y, size.x, size.y};
  }

  float speed() const { return vec_mag(velocity); }

  vec2 center() const {
    return {
        position.x + (size.x / 2.f),
        position.y + (size.y / 2.f),
    };
  }

  raylib::Rectangle focus_rect(int rw = 2) const {
    return raylib::Rectangle{position.x - (float)rw, position.y - (float)rw,
                             size.x + (2.f * (float)rw),
                             size.y + (2.f * (float)rw)};
  }

  float as_rad() const { return static_cast<float>(angle * (M_PI / 180.0f)); }
};
struct ProvidesB2WorldID : BaseComponent {
  b2WorldId id;
  ProvidesB2WorldID(b2WorldId id_) : id(id_) {}
};

struct PlayerID : BaseComponent {
  input::GamepadID id;
  PlayerID(input::GamepadID i) : id(i) {}
};

using raylib::Rectangle;
struct EQ : public EntityQuery<EQ> {
  struct WhereInRange : EntityQuery::Modification {
    vec2 position;
    float range;

    // TODO mess around with the right epsilon here
    explicit WhereInRange(vec2 pos, float r = 0.01f)
        : position(pos), range(r) {}
    bool operator()(const Entity &entity) const override {
      vec2 pos = entity.get<Transform>().pos();
      return distance_sq(position, pos) < (range * range);
    }
  };

  EQ &whereInRange(const vec2 &position, float range) {
    return add_mod(new WhereInRange(position, range));
  }

  EQ &orderByDist(const vec2 &position) {
    return orderByLambda([position](const Entity &a, const Entity &b) {
      float a_dist = distance_sq(a.get<Transform>().pos(), position);
      float b_dist = distance_sq(b.get<Transform>().pos(), position);
      return a_dist < b_dist;
    });
  }

  struct WhereOverlaps : EntityQuery::Modification {
    Rectangle rect;

    explicit WhereOverlaps(Rectangle rect_) : rect(rect_) {}

    bool operator()(const Entity &entity) const override {
      const auto r1 = rect;
      const auto r2 = entity.get<Transform>().rect();
      // Check if the rectangles overlap on the x-axis
      const bool xOverlap = r1.x < r2.x + r2.width && r2.x < r1.x + r1.width;

      // Check if the rectangles overlap on the y-axis
      const bool yOverlap = r1.y < r2.y + r2.height && r2.y < r1.y + r1.height;

      // Return true if both x and y overlap
      return xOverlap && yOverlap;
    }
  };

  EQ &whereOverlaps(const Rectangle r) { return add_mod(new WhereOverlaps(r)); }
};

struct RenderBodies : System<HasBody> {

  virtual void for_each_with(const Entity &, const HasBody &hasBody,
                             float) const override {

    auto pos = b2Body_GetPosition(hasBody.id);
    auto rotation = b2Body_GetRotation(hasBody.id);

    const int32_t MAX_SHAPES = 1;
    b2ShapeId shapes[MAX_SHAPES];
    int shape_count = b2Body_GetShapes(hasBody.id, shapes, MAX_SHAPES);
    if (shape_count <= 0)
      return;
    b2ShapeId shapeid = shapes[0];
    b2Polygon poly = b2Shape_GetPolygon(shapeid);
    b2AABB b = b2ComputePolygonAABB(&poly, b2Transform_identity);
    float width = b.upperBound.x - b.lowerBound.x;
    float height = b.upperBound.y - b.lowerBound.y;

    /*

       b2Vec2 worldCenter = b2Body_GetWorldCenterOfMass(myBodyId);
b2Vec2 localCenter = b2Body_GetLocalCenterOfMass(myBodyId);

     */

    raylib::DrawRectanglePro({pos.x, pos.y, width, height}, {0, 0},
                             as_deg(b2Rot_GetAngle(rotation)), raylib::GREEN);
  }
};

struct RenderEntities : System<Transform> {

  virtual void for_each_with(const Entity &, const Transform &transform,
                             float) const override {
    raylib::DrawRectanglePro(
        transform.rect(),
        vec2{transform.size.x / 2.f,
             transform.size.y / 4.f}, // transform.center(),
        transform.angle, raylib::RAYWHITE);
  }
};

struct Move : System<PlayerID, HasBody, Transform> {

  float max_speed = 5.f;

  virtual void for_each_with(Entity &, PlayerID &playerID, HasBody &hasBody,
                             Transform &transform, float dt) override {

    input::PossibleInputCollector<InputAction> inpc =
        input::get_input_collector<InputAction>();
    if (!inpc.has_value()) {
      return;
    }

    float accel = 0.f;
    float steer = 0.f;

    for (auto &actions_done : inpc.inputs()) {
      if (actions_done.id != playerID.id)
        continue;
      switch (actions_done.action) {
      case InputAction::Accel:
        accel = 2.0f;
        break;
      case InputAction::Brake:
        accel = -0.5f;
        break;
      case InputAction::Left:
        steer = -1.f * actions_done.amount_pressed;
        break;
      case InputAction::Right:
        steer = actions_done.amount_pressed;
        break;
      default:
        break;
      }
    }

    float minRadius = 10.0f;
    float maxRadius = 300.0f;
    float rad = lerp(minRadius, maxRadius, transform.speed() / max_speed);

    float mvt =
        std::max(-max_speed, std::min(max_speed, transform.speed() + accel));

    transform.angle += steer * dt * rad;

    transform.velocity += vec2{
        std::sin(transform.as_rad()) * mvt * dt,
        -std::cos(transform.as_rad()) * mvt * dt,
    };

    transform.position += transform.velocity;
    transform.velocity = transform.velocity * 0.99f;

    b2Body_ApplyForceToCenter(
        hasBody.id,
        {
            200.f,   // std::sin(transform.as_rad()) * mvt * 200.f,
            10000.f, // -std::cos(transform.as_rad()) * mvt * 200.f,
        },
        true);

    // if (playerID.id == 0)
    // b2Body_SetTransform(hasBody.id,
    // b2Vec2{
    // transform.position.x,
    // transform.position.y,
    // },
    // b2MakeRot(transform.as_rad()));
  }
};

void make_player(b2WorldId worldID, input::GamepadID id) {
  auto &entity = EntityHelper::createEntity();

  vec2 position = {.x = id == 0 ? 150.f : 1100.f, .y = 720.f / 2.f};

  entity.addComponent<PlayerID>(id);
  entity.addComponent<Transform>(position, vec2{30.f, 50.f});

  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = b2_dynamicBody;
  bodyDef.position = b2Vec2{position.x, position.y};

  b2BodyId bodyId = b2CreateBody(worldID, &bodyDef);
  b2Polygon dynamicBox = b2MakeBox(30.0f, 50.0f);
  b2ShapeDef shapeDef = b2DefaultShapeDef();
  shapeDef.density = 1.0f;
  shapeDef.friction = 0.3f;
  b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);

  std::cout << b2Body_GetMass(bodyID) << "\n";
  std::cout << b2Body_GetRotationalInertia(bodyID) << "\n";

  entity.addComponent<HasBody>(bodyId);
}

static void load_gamepad_mappings() {
  std::ifstream ifs("gamecontrollerdb.txt");
  if (!ifs.is_open()) {
    std::cout << "Failed to load game controller db" << std::endl;
    return;
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  input::set_gamepad_mappings(buffer.str().c_str());
}

struct MatchKartsToPlayers : System<input::ProvidesMaxGamepadID> {

  virtual void for_each_with(Entity &,
                             input::ProvidesMaxGamepadID &maxGamepadID,
                             float) override {

    auto existing_players = EQ().whereHasComponent<PlayerID>().gen();

    // we are good
    if (existing_players.size() == maxGamepadID.count())
      return;

    if (existing_players.size() > maxGamepadID.count()) {
      // remove the player that left
      for (Entity &player : existing_players) {
        if (input::is_gamepad_available(player.get<PlayerID>().id))
          continue;
        player.cleanup = true;
      }
      return;
    }

    // we need to add a new player

    Entity &sophie =
        EQ().whereHasComponent<ProvidesB2WorldID>().gen_first_enforce();
    auto worldID = sophie.get<ProvidesB2WorldID>().id;
    for (int i = 0; i < (int)maxGamepadID.count(); i++) {
      bool found = false;
      for (Entity &player : existing_players) {
        if (i == player.get<PlayerID>().id) {
          found = true;
          break;
        }
      }
      if (!found) {
        make_player(worldID, i);
      }
    }
  }
};

int main(void) {
  const int screenWidth = 1280;
  const int screenHeight = 720;

  raylib::InitWindow(screenWidth, screenHeight, "kart-afterhours");
  raylib::SetTargetFPS(200);

  load_gamepad_mappings();

  float lengthUnitsPerMeter = 128.0f;
  b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);
  b2WorldDef worldDef = b2DefaultWorldDef();
  worldDef.gravity = b2Vec2{0.f, 0.f};
  b2WorldId worldID = b2CreateWorld(&worldDef);

  // sophie
  {
    auto &entity = EntityHelper::createEntity();
    input::add_singleton_components<InputAction>(entity, get_mapping());
    window_manager::add_singleton_components(entity, 200);
    entity.addComponent<ProvidesB2WorldID>(worldID);
  }

  SystemManager systems;

  make_player(worldID, 0);

  // debug systems
  {
    window_manager::enforce_singletons(systems);
    input::enforce_singletons<InputAction>(systems);
  }

  // external plugins
  {
    input::register_update_systems<InputAction>(systems);
    window_manager::register_update_systems(systems);
  }

  systems.register_update_system(std::make_unique<Move>());
  systems.register_update_system(std::make_unique<MatchKartsToPlayers>());
  systems.register_update_system(
      [worldID](float dt) { b2World_Step(worldID, dt, 4); });

  // renders
  {
    systems.register_render_system(
        [&](float) { raylib::ClearBackground(raylib::DARKGRAY); });
    systems.register_render_system(std::make_unique<RenderFPS>());
    systems.register_render_system(std::make_unique<RenderEntities>());
    systems.register_render_system(std::make_unique<RenderBodies>());
  }

  while (!raylib::WindowShouldClose()) {
    raylib::BeginDrawing();
    { systems.run(raylib::GetFrameTime()); }
    raylib::EndDrawing();
  }

  raylib::CloseWindow();
  b2DestroyWorld(worldID);

  return 0;
}
