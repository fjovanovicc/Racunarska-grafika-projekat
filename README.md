# Racunarska grafika projekat
Projekat napravljen na kursu Racunarska grafika koristeci [OpenGL](https://learnopengl.com/) specifikaciju.  
Projekat napravio: Filip Jovanovic 336/2018  
Na sceni su prikazani modeli Sunca, Meseca i Marsa. 
Raketa polece sa planete Zemlje (Florida) kako bi spasila 2 astronauta koji su ostali na Marsu. 
Modeli rakete i astronauta se nalaze u kutijama nad kojim je odradjen efekat [Blending-a](https://learnopengl.com/Advanced-OpenGL/Blending), odnosno [Face culling-a](https://learnopengl.com/Advanced-OpenGL/Face-culling).

## Interakcija
 - `W`, `A`, `S`, `D` - Kretanje scenom redom napred, levo, nazad, desno
 - `F1` - Meni (GUI)
 - `F1->HDR` - Aktivacija HDR efekta
 - `F1->Bloom` - Aktivacija Bloom efekta
 - `B` - Promena izmedju Fongovog i Blin-Fongovog modela (promena je aktivna samo na metalnoj teksturi ispod kutija tj. maketa)
 
## Resursi
 - Neki od sajtova za preuzimanje modela: [Turbosquid](https://www.turbosquid.com/Search/3D-Models), [Sketchfab](https://sketchfab.com/3d-models), [Artec3D](https://www.artec3d.com/3d-models), [CGtrader](https://www.cgtrader.com/3d-models)
 - Modeli
   - [Astronaut](https://sketchfab.com/3d-models/astronaut-obj-sva-sva-23512-0c780fc92a1c420bb2286cc19a400034) ([`Sketchfab`](https://sketchfab.com/3d-models))
   - [Raketa](https://www.turbosquid.com/3d-models/3d-toy-rocket-4k-free-1973134) ([`Turbosquid`](https://www.turbosquid.com/Search/3D-Models))
   - [Mars](https://www.turbosquid.com/3d-models/realistic-mars-photorealistic-2k-3d-1277433) ([`Turbosquid`](https://www.turbosquid.com/Search/3D-Models))
   - [Zemlja](https://www.turbosquid.com/3d-models/earth-max-free/1016431) ([`Turbosquid`](https://www.turbosquid.com/Search/3D-Models))
   - [Sunce](https://sketchfab.com/3d-models/the-sun-0d28aa65eb174d948c2716d73e8fd3bd) ([`Sketchfab`](https://sketchfab.com/3d-models))
 - Teksture:
   - Skybox (~link~)
   - Ostale teksture su radjene u [Photoshop-u](https://www.adobe.com/products/photoshop.html)

## Neki od problema sa modelima
 - Ako se model ucitava ali se tekstura ne ucitava, proveriti da li postoji mtl fajl
 - Proveriti da li se tekstura nalazi ispod koordinata mapiranja `map_Kd ime_teksture.ext`
 - Proveriti da li su odgovarajuce teksture mapirane na odgovarajuce koordinate
 - Proveriti putanje do tekstura, relativne putanje su u odnosu na putanju mtl fajla

## Implementacija oblasti i ocenjivanje
- Obavezne oblasti:
  - [x] 1-8 nedelje
  - [x] [Blending](https://learnopengl.com/Advanced-OpenGL/Blending) (Maketa rakete u kutiji)
  - [x] [Face culling](https://learnopengl.com/Advanced-OpenGL/Face-culling) (Maketa astronauta u kutiji)
  - [x] [Advanced lighting](https://learnopengl.com/Advanced-Lighting/Advanced-Lighting)
- Oblasti grupe A:
  - [ ] [Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)
  - [x] [Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)
  - [ ] [Instancing](https://learnopengl.com/Advanced-OpenGL/Instancing)
  - [ ] [Anti Aliasing](https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing)
- Oblasti grupe B:
  - [ ] [Point shadows](https://learnopengl.com/Advanced-Lighting/Shadows/Point-Shadows)
  - [ ] [Normal mapping](https://learnopengl.com/Advanced-Lighting/Normal-Mapping), [Parallax mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping)
  - [x] [HDR](https://learnopengl.com/Advanced-Lighting/HDR), [Bloom](https://learnopengl.com/Advanced-Lighting/Bloom)
  - [ ] [Deffered Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)
  - [ ] [SSAO](https://learnopengl.com/Advanced-Lighting/SSAO)

- Ako sadrzi obavezne oblasti(1-8 nedelje, Blending, Face culling, Advanced lighting) max broj poena koji je moguce osvojiti je 15
- Ako sadrzi obavezne oblasti + 1 oblast iz grupe A max broj poena koji je moguce osvojiti je 25
- Ako sadrzi obavezne oblasti + 1 oblast iz grupe A + 1 oblast iz grupe B max broj poena koji je moguce osvojiti je 35
- Ako sadrzi obavezne oblasti + 1 oblast iz grupe A + 2 oblasti iz grupe B max broj poena koji je moguce osvojiti je 40 (5 je bonus)

## Napomene za studente prilikom kreiranja projekta
- Svo osvetljenje na sceni treba da se racuna po Blin-Fongovom modelu [Advanced lighting](https://learnopengl.com/Advanced-Lighting/Advanced-Lighting)
- Dovoljno je da projekat ima jedan tip [Blendinga](https://learnopengl.com/Advanced-OpenGL/Blending)(Discard ili blend)
- Na Github-u mora postojati istorija commit-ova da bi bio pregledan
- U projektu nije dozvoljeno koriscenje .obj modela i tekstura iz glavnog [repozitorijuma](https://github.com/matf-racunarska-grafika/LearnOpenGL)
