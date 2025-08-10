## UI animation ideas

- Map grid
  - Stagger in map cards on screen open (10–60 ms per card)
  - Selection pulse on chosen card (scale 1.0 -> 1.05 -> 1.0)
  - Preview crossfade when switching maps (opacity tween)

- Random "?" card
  - Brief shake before shuffle starts
  - Confetti/pulse when final map lands

- Buttons (all screens)
  - Hover scale 1.0 -> 1.03 (EaseOut)
  - Press scale 0.97 -> 1.0 (EaseOut)
  - Disabled fade to 0.5 opacity

- Panels/screens
  - Slide + fade main panels in/out
  - Stagger child controls on entry

- Round countdown
  - Scale+fade each number (3-2-1) with EaseOutBack
  - Progress ring sweep easing

- Round end screen
  - Stagger player cards; winner glow pulse (~1s)
  - Score number roll-up (0 -> final) over 0.6–1.0s

- Toasts/notifications
  - Slide-from-top + fade in
  - Auto-dismiss with fade-out

- Loading/previews
  - Skeleton shimmer while generating previews
  - Brief brightness pop when preview updates

- Container-level
  - Stagger children for screen entry
  - Exit transitions for whole panels
  - Crossfade/slide between screens; optional shared-element emphasis

- Helpers to add in animation plugin
  - More easing: EaseIn, EaseInOut, Back, Elastic, Bounce, Spring
  - Stagger(children, delay)
  - Interrupt/retarget current animation smoothly
  - Keyframes/timeline clips
  - Sequence/parallel combinators (then, together)
  - repeat(count|loop), yoyo(), cancel(key), is_active(key)

- Implementation notes
  - Per-component runtime: opacity, scale, translate, rotation, color
  - Container wrapper to transform a subtree (screen/panel transitions)

