# Resume de passation - optimisations TGX

Ce document resume le travail recent d'optimisation de TGX afin qu'une autre session Codex puisse reprendre sans refaire les memes recherches.

Etat au moment de ce resume :

- Branche : `feature/multi-directional-lights`
- Commit courant : `af7f778641bbdb63ce4d608300615f47b5f76cc2`
- Commit de nettoyage recent : `af7f778 cleanup: compact validation performance artifacts`
- Working tree : `src/Image.inl` est encore modifie et non committe, volontairement, pour relecture humaine.
- Les artefacts de performance ont ete compactes sous `validation/performance/`.

Fichiers importants :

- Outils reutilisables : `validation/performance/tools/`
- Baselines actuelles : `validation/performance/baselines/current/`
- Index compact des investigations : `validation/performance/investigations/README.md`
- Rapports finaux : `validation/performance/investigations/*/REPORT*.md`

## 1. Regles et philosophie retenues

Plusieurs campagnes ont montre que TGX est tres sensible aux effets locaux :

- Une optimisation peut ameliorer le score global tout en degradant fortement des sous-tests utiles.
- Les cartes ne reagissent pas de la meme facon : Teensy 4.1, RP2350/Pico2, ESP32/Core2 et ESP32-S3/CoreS3 ont des profils tres differents.
- Les gains de layout/code-size sont souvent fragiles et ne doivent pas etre acceptes seuls.
- Les changements approximatifs de rendu doivent etre quantifies. Pixel-perfect est ideal, mais des differences visuelles faibles peuvent etre considerees seulement avec metriques d'erreur et validation de scenes.

Pour toute nouvelle optimisation :

- utiliser les outils de capture dans `validation/performance/tools/` ;
- reutiliser les baselines dans `validation/performance/baselines/current/` quand elles sont valides ;
- analyser les sous-tests detailles, pas seulement le score global ;
- verifier les chemins importants : wire/dots, bilinear texture, Gouraud, flat, textured meshes, exemples reels ;
- ne pas garder un patch source sans validation correctness + benchmark + exemples si le chemin est visible par l'utilisateur.

## 2. Outils et baselines

Les outils d'upload/capture/parsing ont ete conserves dans :

```text
validation/performance/tools/
```

Ils incluent notamment :

- `upload_and_capture.ps1`
- `run_benchmark.ps1`
- `run_benchmark_candidate.ps1`
- `run_example.ps1`
- `parse_benchmark_logs.py`
- `parse_example_telemetry.py`
- aggregateurs de candidats et resultats

Ports utilises pendant les campagnes :

| Carte | Port |
| --- | --- |
| Teensy 4.1 | `COM3` |
| Pico2 / RP2350 | `COM21` |
| Core2 / ESP32 classic | `COM5` |
| CoreS3 / ESP32-S3 | `COM10` |
| Pico W / RP2040 | `COM28`, devenu non fiable puis ignore |

Les baselines actuelles sont dans :

```text
validation/performance/baselines/current/
```

Fichiers principaux :

- `benchmark_global_scores.csv`
- `benchmark_subtests.csv`
- `binary_size.csv`
- `example_telemetry.csv`
- `example_telemetry_summary.csv`
- `core2_image_blit_candidate_benchmark_global.csv`
- `core2_image_blit_candidate_benchmark_subtests.csv`
- `cores3_image_blit_candidate_benchmark_global.csv`
- `cores3_image_blit_candidate_benchmark_subtests.csv`

Ces baselines servent a comparer vite un nouveau patch sans relancer une matrice complete inutilement. Les reruns ne sont necessaires que si le code, les macros, la config build, les outils ou le setup materiel ont change de maniere significative.

## 3. Optimisations deja acceptees / commitees

### 3.1 `TGX_INLINE_ZDIVIDE`

Statut : accepte et deja commite avant le nettoyage des artefacts.

Politique retenue :

- `TGX_INLINE_ZDIVIDE` force sur Teensy 4.x ;
- force sur ESP32-S3 / CoreS3 ;
- force sur RP2350 / Pico2 ;
- non force sur ESP32 classic / Core2 ;
- non force sur RP2040 / Pico W.

Raison :

- `Vec4::zdivide()` est chaud dans les transformations/projection.
- L'inlining force etait benefique sur Teensy 4.x, ESP32-S3 et RP2350.
- Core2 et RP2040 etaient plus sensibles au code-size/layout ; forcer `zdivide()` pouvait regresser.

Conclusion :

- Ne pas refaire cette campagne sauf bug de correctness.
- C'est la baseline actuelle.

### 3.2 Chemin RP2350 shader incremental pixel pointer

Statut : accepte et deja commite avant les campagnes recentes.

Raison :

- RP2350 beneficiait d'une progression de pointeurs plus directe dans certains chemins shader.
- Le changement etait garde par macro/plateforme.

Conclusion :

- Traiter comme baseline.
- Ne pas casser ce chemin lors de futurs travaux shader.

### 3.3 `meanColor(RGB565, RGB565)` exact packed average

Statut : accepte et deja commite.

Formule retenue :

```cpp
(a & b) + (((a ^ b) & 0xF7DEu) >> 1)
```

Raison :

- Exact pour la semantique RGB565 de moyenne de deux couleurs.
- Plus rapide dans les microbenchmarks ARM.
- Code simple, maintenable, sans dependance DSP/CMSIS.

Limite :

- Probablement peu utilise dans le coeur du renderer 3D.
- Gain reel global faible, mais patch propre et exact.

Conclusion :

- Ne pas refaire `meanColor2`.
- Les pistes RGB565 suivantes doivent cibler des fonctions vraiment chaudes dans le rendu 3D ou Image.

## 4. WIP actuel non committe : `src/Image.inl`

Statut : modifie dans le working tree, volontairement non committe pour relecture.

Fichier :

```text
src/Image.inl
```

Fonctions touchees :

- `Image<color_t>::_blitRegionUp(...)`
- `Image<color_t>::_blitRegionDown(...)`

Idee :

- La librairie separait deja `_blitRegionUp` et `_blitRegionDown` pour gerer le sens de copie quand source et destination peuvent se recouvrir.
- Le patch ne supprime pas cette logique.
- Il ajoute seulement un choix plus fin entre `memmove()` et `memcpy()` :
  - `memmove()` reste utilise quand le recouvrement est possible ou quand la copie est minuscule ;
  - `memcpy()` est utilise uniquement quand les plages source/destination sont prouvees disjointes ;
  - `1x1` devient une affectation directe ;
  - regions vides : retour immediat.

Motivation :

- `memmove()` est correct pour overlap, mais plus couteux.
- Dans beaucoup de `Image::blit()` reels, les deux regions ne se recouvrent pas.
- Quand l'absence de recouvrement est prouvee, `memcpy()` donne de gros gains, surtout sur ESP32/Core2 et ESP32-S3/CoreS3.

Gains mesures dans le rapport ESP Image/span :

| Board | Workload | Baseline us | Candidate us | Delta |
| --- | --- | ---: | ---: | ---: |
| Core2 | contiguous `Image_blit` 64x32 | 26105 | 2947 | +785.82% |
| Core2 | rect copy 320x20 | 28511 | 3727 | +664.99% |
| Core2 | mixed workload | 1313269 | 1107937 | +18.53% |
| CoreS3 | contiguous `Image_blit` 64x32 | 22155 | 1508 | +1369.16% |
| CoreS3 | rect copy 320x20 | 22184 | 2099 | +956.88% |
| CoreS3 | mixed workload | 632580 | 535667 | +18.09% |

Validation effectuee pendant la campagne :

- CPU 2D : pass.
- CPU 3D strict : pass, 82 PASS / 0 FAIL.
- Microbench Image wrappers Core2/CoreS3 : production rows correctes.
- Bench TGX Core2/CoreS3 : scores globaux neutres.
- Exemples Core2/CoreS3 (`donkeykong`, `borg_cube`, `scream`) : globalement neutres, avec une limitation de fenetre de capture sur `donkeykong`.

Points a relire :

- Le test d'overlap est fait via bornes globales source/destination. Il est volontairement conservateur.
- Pour les copies multi-lignes, si les rectangles globaux peuvent se recouvrir, toutes les lignes restent en `memmove()`.
- Pour les regions disjointes, les lignes passent en `memcpy()`.
- Le patch est generique `color_t`, pas seulement RGB565.
- A verifier : casts pointeur vers `size_t` et style local TGX ; eventuellement preferer `uintptr_t` si le projet l'utilise/autorise.

Conclusion provisoire :

- C'est la piste source la plus prometteuse actuellement.
- Elle merite une relecture humaine avant commit.

## 5. Campagne config / inline / `tgx_config.h`

Objectif :

- Clarifier et optimiser les macros de configuration :
  - `TGX_INLINE`
  - `TGX_NOINLINE`
  - `TGX_INLINE_ZDIVIDE`
  - fast math macros
  - shader incremental pointer path
  - categories d'inlining plus fines.

Resultats :

- La partie utile a ete `TGX_INLINE_ZDIVIDE`, deja gardee.
- Le reste n'a pas produit de politique robuste a garder.

Pistes testees :

- `TGX_INLINE` global force/empty selon cartes.
- Macros plus fines :
  - `TGX_INLINE_VEC2`
  - `TGX_INLINE_VEC3`
  - `TGX_INLINE_VEC4`
  - `TGX_INLINE_COLOR`
  - `TGX_INLINE_RGBF`
  - `TGX_INLINE_MATH`
  - `TGX_INLINE_SHADER`
  - `TGX_INLINE_IMAGE`
- Fast math toggles :
  - `TGX_USE_FAST_INV_TRICK`
  - `TGX_USE_FAST_SQRT_TRICK`
  - `TGX_USE_FAST_INV_SQRT_TRICK`
  - `TGX_USE_FMA_MATH`

Conclusions :

- Les macros de categorie neutres etaient trop couteuses en review/churn pour peu de valeur immediate.
- Aucun override de categorie n'a survecu aux sous-tests et exemples.
- Les gains d'inlining etaient souvent des effets de layout.

Abandonne / ne pas refaire tel quel :

- Broad inline category layer.
- `TGX_INLINE` global force/empty comme strategie large.
- Changer les fast math macros globalement sans microbench cible.

## 6. Campagne `TGX_INLINE_IMAGE=`

Signal initial :

- Sur Pico2/RP2350, `TGX_INLINE_IMAGE=` a donne environ +4.4% global.

Probleme :

- Le score global cachait de tres grosses regressions locales :
  - `Wire rib tri thick` autour de -19.6%.
  - Point cloud / dots autour de -15%.
  - Plusieurs chemins wire/dots importants etaient touches.

Investigations suivantes :

- Split de `TGX_INLINE_IMAGE` en categories plus fines :
  - accessors ;
  - pixel helpers ;
  - wire/dots ;
  - span/blit ;
  - mesh-safe candidates.

Conclusion :

- Proteger wire/dots supprimait le gain.
- Le gain venait probablement de code-size/layout et non d'une amelioration directe Image.
- Piste rejetee.

Ne pas refaire :

- `TGX_INLINE_IMAGE=` coarse.
- Image inline split pour capturer un gain global Pico2.

Lecon utile :

- Pico2/RP2350 est tres sensible au placement/code-size dans Renderer3D/shader.
- Les sous-tests wire/dots doivent rester des garde-fous.

## 7. Campagne Renderer3D / shader / layout

Objectif :

- Chercher un remplacement direct et stable au faux signal `TGX_INLINE_IMAGE=`.
- Cibler Renderer3D, shader selection, rasterizer, cold/hot split, template bloat, layout.

Pistes testees :

- `cold_path_separation_candidate`
- `cold_shader_selection_candidate`
- `cold_clipping_candidate`
- `shader_select_hot_cold_split`
- `shader_common_fast_path_candidate`
- `texture_path_split_candidate`
- `raster_pointer_increment_candidate`
- `raster_branch_reduction_candidate`
- `raster_invariant_hoist_candidate`
- plusieurs candidats RP2350 mesh layout.

Resultats :

- Quelques gains locaux etaient observes, parfois forts.
- Mais les gains etaient fragiles, suspects de layout, ou accompagnes de regressions locales :
  - wire/dots ;
  - bilinear texture ;
  - flat/Gouraud ;
  - paths mesh utiles.

Conclusion :

- Aucun patch source garde.
- Ne pas poursuivre les perturbations larges Renderer3D/layout sans microbenchmark isolé.

Lecon :

- La bonne prochaine etape est de microbenchmarker des kernels realistes, pas de bouger du code template central au hasard.

## 8. ARM DSP / CMSIS RGB565

Cartes :

- Teensy 4.1 / Cortex-M7 / `COM3`
- Pico2 / RP2350 / Cortex-M33 / `COM21`

Objectif :

- Tester des intrinsics ARM DSP/CMSIS sur les kernels RGB565 :
  - blend ;
  - mean ;
  - bilinear ;
  - triangle interpolation ;
  - RGBf -> RGB565 ;
  - span kernels.

Resultat garde :

- `meanColor(RGB565, RGB565)` exact packed average, deja commite.

Pistes rejetees :

- Intrinsics `UHADD16` naifs : problemes de frontieres de canaux RGB565.
- Variantes blend/mult/bilinear/triangle : scalar deja fort, exactness difficile, gains faibles ou pas sur chemins chauds.
- RGBf -> RGB565 : petites ameliorations isolees mais regressions ou pertinence insuffisante.

Conclusion :

- ARM DSP n'est pas une baguette magique pour RGB565 a cause du packing 5/6/5.
- Les bit tricks scalaires exacts sont souvent meilleurs et plus lisibles.

Ne pas refaire :

- Reprendre les memes intrinsics RGB565 sans nouvelle idee de packing/exactness.

## 9. ARM RGB565 hot 3D

Objectif :

- Se concentrer sur les kernels vraiment appeles par le rendu 3D :
  - `interpolateColorsBilinear(RGB565, ...)`
  - `interpolateColorsTriangle(...)`
  - `mult256` / modulation couleur ;
  - `RGBf -> RGB565` si hot ;
  - `blend256` seulement si vraiment appele.

Conclusion :

- Aucun patch source robuste.
- Les kernels exacts utiles etaient difficiles a ameliorer sans casser la semantique.
- Beaucoup de fonctions colorees sont moins centrales au chemin 3D que suppose au depart.

Ne pas refaire :

- Optimiser `meanColor` comme si c'etait central au 3D.
- Integrer des approximations couleur sans validation visuelle explicite.

## 10. Raster/span microbenchmarks ARM

Objectif :

- Construire un niveau intermediaire entre microbench color-only et renderer complet.
- Modeliser les boucles span :
  - z-test/write ;
  - flat RGB565 ;
  - Gouraud ;
  - texture nearest ;
  - texture bilinear ;
  - texture x light/color ;
  - short spans et long spans.

Resultat important :

- Les coordonnees texture incrementales en fixed-point etaient tres prometteuses.

Gains microbench approximatifs :

- Nearest fixed8/fixed incremental :
  - Teensy : fort gain, autour de +53% dans le premier span bench.
  - Pico2 : autour de +28%.
- Bilinear fixed incremental :
  - Teensy : autour de +15%.
  - Pico2 : autour de +12%.

Limite :

- Le microbench utilisait un modele controle de coordonnees fixed8, pas toute la semantique reelle TGX :
  - perspective correction ;
  - clamp/wrap ;
  - bords texture ;
  - arrondis exacts ;
  - bilinear fractions.

Conclusion :

- Pas de patch source.
- Piste a creuser avec equivalence semantique plus stricte.

## 11. Texture coordinates fixed-point

Objectif :

- Verifier si les coordonnees texture incrementales fixed-point peuvent correspondre a la semantique shader reelle.

Safe subset identifie :

- `USE_TEXTURE && USE_ORTHO`
- affine / non-perspective
- span entier dans l'interieur texture
- fixed 16 fractional bits
- longueur `len >= 8`
- nearest : exact dans le subset teste
- bilinear : approximatif mais borne, max erreur canal RGB565 = 1 dans les tests acceptes

Gains equivalence benchmark pour `len >= 8` :

| Mode | Teensy 4.1 | Pico2 |
| --- | ---: | ---: |
| nearest | environ +138.6% | environ +108.5% |
| bilinear | environ +56.0% | environ +38.5% |

Perspective :

- Approximation fixed perspective rejetee.
- Erreurs trop grandes, jusqu'a max channel error 63 dans certains tests.

Conclusion :

- Piste prometteuse mais pas encore integrable directement.
- Il fallait un patch shader avec guards/fallbacks reels.

## 12. Shader fixed-point integration

Objectif :

- Integrer un fast path `src/Shaders.h` pour le subset affine/interieur.

Strategie testee :

- nearest-only d'abord ;
- `USE_TEXTURE && USE_ORTHO` ;
- nearest clamp/interior ;
- fallback float pour perspective, wrap, bilinear, edges ;
- seuil de longueur ;
- fixed16.

Validation render-diff :

- Nearest clamp/interior exact dans les tests Teensy/Pico2.
- Nearest wrap non exact dans certains cas.
- Bilinear borne dans le sketch, mais pas candidate initiale.

Benchmark TGX :

- Quelques lignes nearest gagnaient :
  - `R2D2 TEX_NEAREST` +4% environ ;
  - `Bunny TEX_NEAREST` +4-5% environ.
- Mais regressions cachees severes :
  - bilinear texture rows jusqu'a -40% dans certains candidats ;
  - meme les candidats narrowed restaient negatifs sur global/sous-tests.

Conclusion :

- Patch shader nearest rejete.
- Bilinear non tente en production car nearest n'a pas passe les gates.
- Ne pas accepter un fast path shader central sur microbench seul.

Piste future :

- Reprendre seulement avec une integration encore plus isolee qui ne perturbe pas les instanciations bilinear.
- Possiblement separer les templates ou creer un chemin compile-time vraiment distinct, mais attention au layout.

## 13. ESP32 / ESP32-S3 kernels

Cartes :

- Core2 / ESP32 classic / Xtensa LX6 / `COM5`
- CoreS3 / ESP32-S3 / Xtensa LX7 / `COM10`

Objectif :

- Chercher des opportunites ESP-specifiques :
  - Xtensa reciprocal asm ;
  - ESP32-S3 SIMD/PIE ;
  - RGB565 packed kernels ;
  - span copy/fill ;
  - fixed16 texture coords ;
  - IRAM placement.

Math / reciprocal :

- `tgx::fast_inv()` actuel est deja le bon pattern safe `recip0.s` + deux Newton steps.
- Un step est plus rapide mais approximatif ; non integre.
- Deplacer l'asm au call-site n'a pas justifie un patch.

IRAM :

- Placement `IRAM_ATTR` neutre sur Core2, souvent negatif sur CoreS3.
- Ne pas poursuivre broad IRAM placement.

RGB565 kernels :

- Quelques gains isoles :
  - `RGB565::mult256` extract-scalar legerement mieux sur Core2 ;
  - `RGBf -> RGB565` manuel exact un peu mieux en isolation.
- Pas assez pertinent/robuste pour patch source.

Span copy/fill :

- Gros signaux microbench :
  - `memcpy`, `dsps_memcpy`, stores 32-bit, unroll.
  - CoreS3 `dsps_memcpy` tres fort sur longues copies.
- Mais besoin de seuils et verification de call sites Image.

Fixed16 texture coords sur ESP :

- Prometteur en microbench.
- Non integre dans cette session ; meme probleme semantique shader que ARM.

Conclusion :

- Aucun patch source depuis cette campagne large ESP.
- La piste Image/span copy/fill a ete separee en campagne dediee.

## 14. ESP32-S3 / ESP32 Image span copy/fill/blit

Objectif :

- Passer du microbench raw span a des vrais call sites `Image`.
- Cibler `Image::blit`, fill/copy/blit RGB565, rectangles, workloads mixtes.

Audit source :

- Les fills RGB565 etaient deja optimises via `_fast_memset()` :
  - stores 32-bit / deux pixels ;
  - unroll.
- Les vertical lines ne sont pas des spans contigus.
- Les blits avec blending/mask sont semantiques per-pixel, pas de simple copy.
- Le vrai target utile : same-format opaque `Image::blit()`.

Pistes testees :

- fill RGB565 span :
  - scalar ;
  - 32-bit store ;
  - unroll ;
  - memset selon pattern ;
  - pas retenu car TGX avait deja un bon fill path.
- copy RGB565 span :
  - scalar ;
  - `memcpy` ;
  - `memmove` ;
  - `dsps_memcpy` ;
  - unroll / 32-bit copy ;
  - overlap tests.
- rectangle fill/copy ;
- workloads mixtes.

Rejete :

- `dsps_memcpy()` production :
  - tres rapide sur CoreS3 long spans ;
  - mais pas overlap-safe ;
  - dependance ESP-DSP ;
  - seuils plus complexes ;
  - standard `memcpy()` suffisait pour gros gains Image wrapper.
- raw `memcpy()` sur overlap :
  - correctness fail volontaire dans diagnostics.
- fill changes :
  - deja optimise.
- manual 32-bit copy/unroll :
  - moins simple et pas meilleur que `memcpy`.

Patch WIP actuel :

- Voir section 4.
- Semantique : garder `memmove()` quand overlap possible, utiliser `memcpy()` uniquement quand disjoint.

Conclusion :

- C'est la seule optimisation source actuelle a relire.

## 15. Pistes abandonnees explicitement

Ne pas relancer sans nouvelle hypothese forte :

1. Broad category inline macros.
   - Trop de churn, pas de gain robuste.

2. `TGX_INLINE_IMAGE=`.
   - Gain global Pico2 reel mais regressions wire/dots inacceptables.

3. Direct Renderer3D/layout perturbations.
   - Trop fragile, regressions locales.

4. Fast math global toggles.
   - Risque numerique/layout, peu de gains robustes.

5. IRAM placement large ESP.
   - Neutre ou negatif.

6. DSP RGB565 naif.
   - Problemes exactness RGB565, scalar souvent meilleur.

7. Shader fixed-point fast path central sans isolation.
   - Microbench prometteur mais benchmark TGX regressif.

8. `dsps_memcpy()` comme remplacement generique.
   - Trop risqué pour overlap et petits spans.

## 16. Pistes prometteuses non explorees ou pas encore terminees

### 16.1 Finaliser/revoir le patch `Image::blit()`

Priorite haute.

Actions recommandees :

- Relire `src/Image.inl`.
- Verifier style, casts pointeurs, seuil `8 bytes`.
- Eventuellement tester sur Teensy/Pico2 pour s'assurer que le patch generique ne regresse pas ARM.
- Si accepte : commit dedie.

### 16.2 JPEG / row import RGB565

La campagne ESP Image a note que certains chemins d'import/copie de lignes pourraient beneficier d'un helper copy non-overlap.

Pourquoi prometteur :

- `memcpy()`/long row copy ont donne de tres gros gains.
- Ce serait possiblement un chemin Image/2D reel.

Attention :

- Verifier source/destination contigus.
- Verifier overlap impossible.
- Mesurer avec workloads reels.

### 16.3 ESP32-S3 long non-overlap row helper avec `dsps_memcpy()`

Prometteur mais non integre.

Conditions minimales :

- CoreS3 only ;
- non-overlap prouve ;
- longueur au-dessus d'un seuil conservateur ;
- fallback `memcpy()`/scalar ;
- pas de dependance compliquee ou build fragile.

### 16.4 Shader texture fixed-point, integration plus isolee

Prometteur en microbench/equivalence mais rejete en integration initiale.

Piste :

- Creer un chemin compile-time ou helper qui n'ajoute pas de branches/layout dans les instanciations bilinear.
- Tester d'abord nearest clamp affine interior.
- Garder bilinear pour plus tard.

Risques :

- Regressions bilinear cachees.
- Explosion template/layout.
- Edge/wrap/perspective correctness.

### 16.5 Microbench texture address generation plus petit

Au lieu d'integrer tout le shader :

- isoler seulement address generation ;
- verifier si une petite optimisation peut etre appliquee sans toucher sampling/color/modulation.

### 16.6 ESP32-S3 SIMD/PIE plus profond

La disponibilite/outillage n'a pas donne de patch.

Piste :

- travailler avec exemples minimaux ESP32-S3 SIMD/PIE ;
- cibler spans longs seulement ;
- ne pas faire de fonctions par pixel.

### 16.7 ARM / RP2350 Vec3-Mat4-FMA microbench

Pas encore explore en profondeur apres les RGB565/span sessions.

Candidats :

- transform vertex ;
- dot/cross/normalize ;
- Mat4 x Vec4 ;
- projection sans changer precision visible.

### 16.8 Image copy/fill/blit sur ARM

Le patch `Image::blit()` est generique et pourrait aider aussi ARM.

A faire si le patch est retenu :

- microbench Image wrapper Teensy/Pico2 ;
- exemples 2D/Image si disponibles ;
- CPU validation deja OK dans campagne ESP, mais refaire apres rebase/commit final si necessaire.

## 17. Conseils pour l'autre session Codex

Commencer par :

1. Lire ce fichier.
2. Lire `validation/performance/investigations/README.md`.
3. Lire `validation/performance/investigations/2026-06-esp32s3-image-span-copyfill/REPORT_ESP32S3_IMAGE_SPAN_COPYFILL.md`.
4. Inspecter `git diff -- src/Image.inl`.
5. Decider si le patch `Image.inl` est acceptable, a modifier, ou a abandonner.

Ne pas commencer par :

- relancer toutes les baselines ;
- refaire les inline macro experiments ;
- refaire `TGX_INLINE_IMAGE=`;
- refaire les microbench RGB565 ARM deja conclus ;
- toucher `Shaders.h` pour fixed-point sans nouvelle strategie d'isolation.

Commandes utiles :

```powershell
git status --short
git diff -- src/Image.inl
Get-Content validation\performance\baselines\current\README.md
Get-Content validation\performance\investigations\README.md
```

Si une nouvelle optimisation est testee :

- creer un dossier date sous `validation/performance/investigations/`;
- garder les raw logs temporaires seulement pendant la session ;
- a la fin, conserver un rapport compact et les CSV vraiment reutilisables ;
- ne pas recommitter de gros dumps/builds/raw captures.

## 18. Etat rapide des conclusions

| Sujet | Verdict |
| --- | --- |
| `TGX_INLINE_ZDIVIDE` | Garde, deja baseline |
| RP2350 incremental shader pointer | Garde, deja baseline |
| `meanColor2` packed RGB565 | Garde, deja committe |
| Broad inline categories | Rejete |
| `TGX_INLINE_IMAGE=` | Rejete, regressions wire/dots |
| Renderer/layout broad changes | Rejete, fragile |
| ARM DSP RGB565 general | Rejete sauf `meanColor2` |
| Hot 3D RGB565 kernels | Pas de patch |
| Raster span fixed coords | Prometteur, pas integre |
| Texture fixed16 equivalence | Safe subset trouve, pas integre |
| Shader fixedpoint integration | Rejete pour l'instant |
| ESP math/IRAM/RGB565 broad | Pas de patch |
| ESP Image blit memcpy/memmove | WIP actuel prometteur dans `src/Image.inl` |

