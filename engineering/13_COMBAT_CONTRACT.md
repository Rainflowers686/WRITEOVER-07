# 13_COMBAT_CONTRACT (REPAIR PASS v2)

> 修订：统一到冻结类型——combat 公共类型见 `player/combat.h` + `common/weapon_types.h`
> （强 ID、std::array 弹药、typed FireRequest/HitscanResult；武器名是 StringId 而非
> `const char*`；事件载荷用 `EventDamage/EventWeaponFire` typed payload，不手工
> 拼 `uint64_t` 字段）。命中判定走 `IWorldQuery`（read-only），damage 全 hitscan。

## Hitscan Flow (per shot)

1. Check ammo & fire rate & near-wall rule.
2. Compute aim direction = camera yaw/pitch + spread offset (sim RNG).
3. Cast hitscan ray against grid (IWorldQuery read-only; first blocked interval at
   eye height is a static hit).
4. If NPC hit: compute damage (headshot ×2 for kinetic), post `EventDamage`.
5. Apply recoil to camera; decrement ammo; post `EventWeaponFire` → AI hears.

```cpp
struct HitscanResult {
    bool hit = false;
    EntityId target_id;     // invalid = static world geometry
    Vec3 hit_point;
    float distance = 0.0f;
    bool headshot = false;
    uint16_t damage = 0;
};
HitscanResult ResolveHitscan(const FireRequest& req, const WeaponDef& weapon,
                             const IWorldQuery& world, DeterministicRNG& sim_rng);
```

## Damage / Death

```cpp
struct EventDamage {        // common/world_event.h (typed payload)
    EntityId target; EntityId source; uint16_t amount;
    uint8_t damage_type; bool headshot;
};
```

// Damage application rules:
// - Player HP 0-100. On 0 → death, post PlayerDeath, trigger respawn/checkpoint logic.
// - NPC HP: Full 100, Medium 60, Light 40, Guard 50.
// - Stunner: non-lethal, applies stun timer (FSM → Stunned, 5s), not damage.
// - Headshot: ×2 for kinetic.
```

## Ammo / Reload

- Reload requires: magazine not full AND reserve > 0.
- Reload cancels on: fire pressed (reload-then-fire debounce), posture change to prone? (simplification: cancel on posture change), weapon switch.
- Reload state in frames (`reloadTime * 120`).

## Near-Wall Rules

- Weapon lowered if any cell within 0.5m in front of camera is solid at weapon height.
- ADS disabled when lowered.
- Fire delayed 0.15s when transitioning from lowered (raising weapon).
- Implemented as capabilities: `CanFire(ls, grid)` + `GetWeaponPose(ls, grid)`.

## Spread / Recoil (Deterministic)

- Base spread per weapon; grows additively per continuous shot; decays at fixed rate.
- Recoil: fixed yaw/pitch increments applied per shot with slight sim-RNG jitter (all deterministic).
- Same input + same seed → same spread/recoil sequence.

## Damage Formula (Frozen)

```
damage = weapon.damage
if (headshot && kinetic) damage *= 2
if (npc.faction == player faction && nonLethal) damage = 0 (friendly)
distance falloff: none inside 0.6*range, linear to 0.5× at range
```

## Audio Events for AI

- `EventWeaponFire` carries loudness by weapon (pistol 0.7, SMG 0.9, stunner 0.4) and origin.
- AI perception consumes these events; guards within radius enter alert.

## Combat Tests

1. Hitscan hits NPC at 7m → correct damage/feedback
2. Near-wall → weapon lowered, fire disabled
3. Reload cancels correctly
4. Deterministic spread: same seed replays identical hit pattern
5. Stunner → NPC stun state, no HP loss

## 报告友好

**STL**: std::array for ammo slots, std::optional for result.
**Design Pattern**: Strategy (weapon type behaviors), State (reload phase).
**Core Algorithm**: Ray-vs-AABB hit test (hitscan), deterministic spread sampling.
**Course Note**: 全武器为即时命中（hitscan），射击只有"命中/未命中/爆头"三种反馈，是终端 FPS 的合理简化。