

#include "makers.h"

#include "components.h"
#include "round_settings.h"
#include "tags.h"

using namespace afterhours;
using afterhours::texture_manager::HasAnimation;
using afterhours::texture_manager::idx_to_sprite_frame;

void make_explosion_anim(afterhours::Entity &parent) {
  const Transform &parent_transform = parent.get<Transform>();

  auto &poof = afterhours::EntityHelper::createEntity();
  poof.addComponent<Transform>(parent_transform.pos(), vec2{10.f, 10.f});

  const Transform &transform = poof.get<Transform>();
  poof.addComponent<HasAnimation>(
      HasAnimation::Params{.position = transform.position,
                           .size = transform.size,
                           .angle = transform.angle,
                           .start_position = vec2{0, 3},
                           .total_frames = 9,
                           .frame_dur = 1.f / 20.f,
                           .once = true,
                           .scale = 2.f,
                           .cur_frame = 0,
                           .rotation = 0,
                           .colorTint = raylib::RAYWHITE});
}

void make_poof_anim(afterhours::Entity &parent, const Weapon &wp, float angle_offset) {
  const Transform &parent_transform = parent.get<Transform>();

  vec2 off;
  float angle = 0;
  switch (wp.firing_direction) {
  case Weapon::FiringDirection::Forward:
    off = {0.f, 0.f};
    angle = 0.f;
    break;
  case Weapon::FiringDirection::Left:
    off = {
        -20.f,
        10.f,
    };
    angle = -90.f;
    break;
  case Weapon::FiringDirection::Right:
    off = {
        20.f,
        10.f,
    };
    angle = 90.f;
    break;
  case Weapon::FiringDirection::Back:
    off = {0.f, 0.f};
    angle = 180.f;
    break;
  }

  auto &poof = afterhours::EntityHelper::createEntity();
  poof.addComponent<TracksEntity>(parent.id, off);
  poof.addComponent<Transform>(parent_transform.pos() + off, vec2{10.f, 10.f})
      .set_angle(parent_transform.angle + angle_offset);
  const Transform &transform = poof.get<Transform>();
  poof.addComponent<HasAnimation>(
      HasAnimation::Params{.position = transform.position,
                           .size = transform.size,
                           .angle = transform.angle,
                           .start_position = vec2{0, 0},
                           .total_frames = 14,
                           .frame_dur = 1.f / 20.f,
                           .once = true,
                           .scale = 1.f,
                           .cur_frame = 0,
                           .rotation = angle,
                           .colorTint = raylib::RAYWHITE});
}

void make_bullet(afterhours::Entity &parent, const Weapon &wp, float angle_offset) {
  const Transform &transform = parent.get<Transform>();

  auto angle = 0.f;
  switch (wp.firing_direction) {
  case Weapon::FiringDirection::Forward:
    break;
  case Weapon::FiringDirection::Left:
    angle = -90.f;
    break;
  case Weapon::FiringDirection::Right:
    angle = 90.f;
    break;
  case Weapon::FiringDirection::Back:
    angle = 180.f;
    break;
  }

  if (wp.config.spread > 0.f) {
    // TODO: improve RNG seeding/determinism for projectile spread
    std::mt19937_64 rng;
    const auto timeSeed =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32)};
    rng.seed(ss);
    std::uniform_real_distribution<float> unif(-wp.config.spread,
                                               wp.config.spread);
    angle_offset += wp.config.size.x * unif(rng);
  }

  vec2 spawn_bias{0, wp.config.size.y};
  auto bullet_spawn_pos = transform.pos() + spawn_bias;

  auto &bullet = afterhours::EntityHelper::createEntity();
  bullet.addComponent<Transform>(bullet_spawn_pos, wp.config.size)
      .set_angle(angle_offset);

  bullet.addComponent<CanDamage>(parent, wp.config.base_damage);
  bullet.addComponent<CollisionAbsorber>(
      CollisionAbsorber::AbsorberType::Absorbed, parent.id);
  bullet.addComponent<HasLifetime>(wp.config.life_time_seconds);

  // If the config doesn't want wrapping,
  // this will make the bullet go into the "void"
  auto wrap_padding =
      wp.config.can_wrap_around ? 0.f : std::numeric_limits<float>::max();
  bullet.addComponent<CanWrapAround>(wrap_padding);

  bullet.addComponent<HasEntityIDBasedColor>(
      parent.id, parent.get<HasColor>().color(), raylib::RED);

  const auto rad = transform.as_rad() + to_radians(angle + angle_offset);
  auto &bullet_transform = bullet.get<Transform>();

  bullet_transform.collision_config =
      CollisionConfig{.mass = 1.f, .friction = 0.f, .restitution = 0.f};

  bullet_transform.velocity =
      vec2{std::sin(rad) * wp.config.speed, -std::cos(rad) * wp.config.speed};

  bullet_transform.accel = wp.config.acceleration;

  bullet_transform.render_out_of_bounds =
      wp.config.can_wrap_around && wp.config.render_out_of_bounds;

  bullet_transform.cleanup_out_of_bounds = !wp.config.can_wrap_around;
}

void make_poof_anim(afterhours::Entity &parent, Weapon::FiringDirection dir,
                    float base_angle, float angle_offset) {
  const Transform &parent_transform = parent.get<Transform>();

  vec2 off;
  float angle = 0.f;
  switch (dir) {
  case Weapon::FiringDirection::Forward:
    off = {0.f, 0.f};
    angle = 0.f;
    break;
  case Weapon::FiringDirection::Left:
    off = {-20.f, 10.f};
    angle = -90.f;
    break;
  case Weapon::FiringDirection::Right:
    off = {20.f, 10.f};
    angle = 90.f;
    break;
  case Weapon::FiringDirection::Back:
    off = {0.f, 0.f};
    angle = 180.f;
    break;
  }

  auto &poof = afterhours::EntityHelper::createEntity();
  poof.addComponent<TracksEntity>(parent.id, off);
  poof.addComponent<Transform>(parent_transform.pos() + off, vec2{10.f, 10.f})
      .set_angle(base_angle + angle_offset);
  const Transform &transform = poof.get<Transform>();
  poof.addComponent<HasAnimation>(
      HasAnimation::Params{.position = transform.position,
                           .size = transform.size,
                           .angle = transform.angle,
                           .start_position = vec2{0, 0},
                           .total_frames = 14,
                           .frame_dur = 1.f / 20.f,
                           .once = true,
                           .scale = 1.f,
                           .cur_frame = 0,
                           .rotation = angle,
                           .colorTint = raylib::RAYWHITE});
}

void make_bullet(afterhours::Entity &parent, const ProjectileConfig &cfg,
                 Weapon::FiringDirection dir, float angle_offset) {
  const Transform &transform = parent.get<Transform>();

  auto angle = 0.f;
  switch (dir) {
  case Weapon::FiringDirection::Forward:
    break;
  case Weapon::FiringDirection::Left:
    angle = -90.f;
    break;
  case Weapon::FiringDirection::Right:
    angle = 90.f;
    break;
  case Weapon::FiringDirection::Back:
    angle = 180.f;
    break;
  }

  float final_angle_offset = angle_offset;
  if (cfg.spread > 0.f) {
    // TODO: improve RNG seeding/determinism for projectile spread
    std::mt19937_64 rng;
    const auto timeSeed =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32)};
    rng.seed(ss);
    std::uniform_real_distribution<float> unif(-cfg.spread, cfg.spread);
    final_angle_offset += cfg.size.x * unif(rng);
  }

  vec2 spawn_bias{0, cfg.size.y};
  auto bullet_spawn_pos = transform.pos() + spawn_bias;

  auto &bullet = afterhours::EntityHelper::createEntity();
  bullet.addComponent<Transform>(bullet_spawn_pos, cfg.size)
      .set_angle(final_angle_offset);

  bullet.addComponent<CanDamage>(parent, cfg.base_damage);
  bullet.addComponent<CollisionAbsorber>(
      CollisionAbsorber::AbsorberType::Absorbed, parent.id);
  bullet.addComponent<HasLifetime>(cfg.life_time_seconds);

  auto wrap_padding =
      cfg.can_wrap_around ? 0.f : std::numeric_limits<float>::max();
  bullet.addComponent<CanWrapAround>(wrap_padding);

  bullet.addComponent<HasEntityIDBasedColor>(
      parent.id, parent.get<HasColor>().color(), raylib::RED);

  const auto rad = transform.as_rad() + to_radians(angle + final_angle_offset);
  auto &bullet_transform = bullet.get<Transform>();

  bullet_transform.collision_config =
      CollisionConfig{.mass = 1.f, .friction = 0.f, .restitution = 0.f};

  bullet_transform.velocity =
      vec2{std::sin(rad) * cfg.speed, -std::cos(rad) * cfg.speed};

  bullet_transform.accel = cfg.acceleration;

  bullet_transform.render_out_of_bounds =
      cfg.can_wrap_around && cfg.render_out_of_bounds;

  bullet_transform.cleanup_out_of_bounds = !cfg.can_wrap_around;
}

afterhours::Entity &make_car(size_t id) {
  auto &entity = afterhours::EntityHelper::createEntity();

  int starting_lives = RoundManager::get().fetch_num_starting_lives();
  entity.addComponent<HasMultipleLives>(starting_lives);
  entity.addComponent<HasKillCountTracker>();
  entity.addComponent<HasTagAndGoTracking>();
  entity.addComponent<HasHippoCollection>();

  auto &transform = entity.addComponent<Transform>(
      get_spawn_position((size_t)id), CarSizes::NORMAL_CAR_SIZE);

  transform.collision_config =
      CollisionConfig{.mass = 1000.f, .friction = .75f, .restitution = .05f};

  entity.addComponent<CollisionAbsorber>(
      CollisionAbsorber::AbsorberType::Absorber);
  entity.addComponent<CanWrapAround>();
  entity.addComponent<HasHealth>(MAX_HEALTH);
  entity.addComponent<TireMarkComponent>();
  entity.addComponent<HasColor>([&entity]() -> raylib::Color {
    return afterhours::EntityHelper::get_singleton_cmp<ManagesAvailableColors>()
        ->get_next_available(entity.id);
  });
  entity.addComponent<afterhours::texture_manager::HasSprite>(
      transform.position, transform.size, transform.angle,
      idx_to_sprite_frame(0, 1), 1.f, entity.get<HasColor>().color());

  entity.addComponent<HasShader>(ShaderType::car);

  auto &enabled_weapons = RoundManager::get().get_enabled_weapons();
  auto &can_shoot = entity.addComponent<CanShoot>();

  // Smart weapon assignment - find available slots
  std::vector<Weapon::Type> weapons_to_assign;

  if (enabled_weapons.test(static_cast<size_t>(Weapon::Type::Cannon))) {
    weapons_to_assign.push_back(Weapon::Type::Cannon);
  }
  if (enabled_weapons.test(static_cast<size_t>(Weapon::Type::Sniper))) {
    weapons_to_assign.push_back(Weapon::Type::Sniper);
  }
  if (enabled_weapons.test(static_cast<size_t>(Weapon::Type::Shotgun))) {
    weapons_to_assign.push_back(Weapon::Type::Shotgun);
  }
  if (enabled_weapons.test(static_cast<size_t>(Weapon::Type::MachineGun))) {
    weapons_to_assign.push_back(Weapon::Type::MachineGun);
  }

  // Fallback to default weapons if none are enabled
  if (weapons_to_assign.empty()) {
    weapons_to_assign.push_back(Weapon::Type::Shotgun);
    weapons_to_assign.push_back(Weapon::Type::MachineGun);
  }

  // Assign weapons to available slots
  std::vector<InputAction> available_slots = {InputAction::ShootLeft,
                                              InputAction::ShootRight};

  for (size_t i = 0; i < weapons_to_assign.size() && i < available_slots.size();
       ++i) {
    can_shoot.register_weapon(available_slots[i],
                              Weapon::FiringDirection::Forward,
                              weapons_to_assign[i]);
  }

  return entity;
}

afterhours::Entity &make_obstacle(raylib::Rectangle rect, const raylib::Color color,
                      const CollisionConfig &collision_config) {
  auto &entity = afterhours::EntityHelper::createEntity();

  auto &transform = entity.addComponent<Transform>(std::move(rect));
  transform.collision_config = collision_config;

  entity.addComponent<CanWrapAround>();
  entity.addComponent<HasColor>(color);
  entity.addComponent<CollisionAbsorber>(
      CollisionAbsorber::AbsorberType::Absorber);
  entity.enableTag(GameTag::MapGenerated);
  // TODO create a rock or other random obstacle sprite
  // entity.addComponent<afterhours::texture_manager::HasSprite>(
  //     transform.position, transform.size, transform.angle,
  //     idx_to_sprite_frame(0, 1), 1.f, entity.get<HasColor>().color());

  return entity;
}

void make_player(input::GamepadID id) {
  auto &entity = make_car(id);
  entity.addComponent<PlayerID>(id);
  entity.addComponent<HonkState>();

  // Add team assignment if team mode is enabled
  if (RoundManager::get().get_active_settings().team_mode_enabled) {
    int team_id = (id % 2 == 0) ? 0 : 1; // Alternate teams
    entity.addComponent<TeamID>(team_id);
  }

  const auto player_id_text = "[Player " + std::to_string(id) + "]";
  const auto player_label_pos_offset = vec2{-.1f, 0.f};
  LabelInfo player_text_label_info(player_id_text, player_label_pos_offset,
                                   LabelInfo::LabelType::StaticText);

  const auto velocity_unit_text = " m/s";
  const auto velocity_label_pos_offset = vec2{2.25f, 1.0f};
  LabelInfo velocity_text_label_info(velocity_unit_text,
                                     velocity_label_pos_offset,
                                     LabelInfo::LabelType::VelocityText);

  const auto acceleration_unit_text = " m/s^2";
  const auto acceleration_label_pos_offset = vec2{2.25f, 2.0f};
  LabelInfo acceleration_text_label_info(
      acceleration_unit_text, acceleration_label_pos_offset,
      LabelInfo::LabelType::AccelerationText);

  std::vector<LabelInfo> player_labels{std::move(player_text_label_info),
                                       std::move(velocity_text_label_info),
                                       std::move(acceleration_text_label_info)};

  entity.addComponent<HasLabels>(std::move(player_labels));
}

void make_ai() {
  // force merge because we are creating entities not inside a system
  // and theres an ent query inside
  size_t num_players = EntityQuery({.force_merge = true})
                           .whereHasComponent<PlayerID>()
                           .gen_count();
  size_t num_ais = EntityQuery({.force_merge = true})
                       .whereHasComponent<AIControlled>()
                       .gen_count();
  auto &entity = make_car(num_players + num_ais);
  entity.addComponent<AIControlled>();
  entity.addComponent<AIDifficulty>();
  entity.addComponent<AIMode>();
  entity.addComponent<AIParams>();

  // Add team assignment if team mode is enabled
  if (RoundManager::get().get_active_settings().team_mode_enabled) {
    int team_id = ((num_players + num_ais) % 2 == 0) ? 0 : 1; // Alternate teams
    entity.addComponent<TeamID>(team_id);
  }
}

afterhours::Entity &make_hippo_item(vec2 position) {
  auto &entity = afterhours::EntityHelper::createEntity();

  entity.addComponent<Transform>(position, vec2{30, 30});
  entity.addComponent<HippoItem>(0.0f);
  entity.addComponent<HasColor>(raylib::GOLD);
  entity.addComponent<CollisionAbsorber>(
      CollisionAbsorber::AbsorberType::Absorbed);

  return entity;
}

afterhours::Entity &make_oil_slick(raylib::Rectangle rect, float steering_multiplier,
                       float acceleration_multiplier,
                       float steering_sensitivity_increment) {
  raylib::Color darker_oil{20, 12, 6, 255};
  auto &entity = afterhours::EntityHelper::createEntity();

  auto &transform = entity.addComponent<Transform>(std::move(rect));
  transform.collision_config =
      CollisionConfig{.mass = std::numeric_limits<float>::max(),
                      .friction = 0.f,
                      .restitution = 0.f};

  entity.addComponent<CanWrapAround>();
  entity.enableTag(GameTag::MapGenerated);
  entity.addComponent<HasColor>(darker_oil);
  entity.enableTag(GameTag::FloorOverlay);
  entity.addComponent<SteeringAffector>(steering_multiplier);
  entity.addComponent<AccelerationAffector>(acceleration_multiplier);
  entity.addComponent<SteeringIncrementor>(steering_sensitivity_increment);

  return entity;
}

afterhours::Entity &make_default_oil_slick(raylib::Rectangle rect) {
  return make_oil_slick(rect, 1.1f, 0.1f, 2.0f);
}

afterhours::Entity &make_sticky_goo(raylib::Rectangle rect) {
  raylib::Color goo{57, 255, 20, 255};
  auto &entity = afterhours::EntityHelper::createEntity();

  auto &transform = entity.addComponent<Transform>(std::move(rect));
  transform.collision_config =
      CollisionConfig{.mass = std::numeric_limits<float>::max(),
                      .friction = 0.f,
                      .restitution = 0.f};

  entity.addComponent<CanWrapAround>();
  entity.enableTag(GameTag::MapGenerated);
  entity.addComponent<HasColor>(goo);
  entity.enableTag(GameTag::FloorOverlay);
  entity.addComponent<SpeedAffector>(0.95f);

  return entity;
}
