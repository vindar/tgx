#ifndef TGX_BENCHMARK_MULTILIGHT_HELPERS_H
#define TGX_BENCHMARK_MULTILIGHT_HELPERS_H

template<typename RendererT>
FLASHMEM void configureDirectionalLightBenchmarkRenderer(RendererT& r)
    {
    const int slx = im.lx();
    const int sly = im.ly();
    r.setViewportSize(slx, sly);
    r.setOffset(0, 0);
    r.setImage(&im);
    r.setZbuffer(zbuf);
    r.setPerspective(45, ((float)slx) / sly, 1.0f, 300.0f);
    r.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64);
    r.setDirectionalLightCount(1);
    r.setLight(fVec3(-1.0f, -1.0f, -1.0f),
               RGBf(1.0f, 1.0f, 1.0f),
               RGBf(1.0f, 1.0f, 1.0f),
               RGBf(1.0f, 1.0f, 1.0f));
    }


template<typename RendererT>
FLASHMEM void setProjectionForDirectionalLightBenchmark(RendererT& r, bool ortho)
    {
    (void)ortho;
    const float aspect = ((float)im.lx()) / im.ly();
    r.setPerspective(45, aspect, 1.0f, 300.0f);
    }


template<typename RendererT>
FLASHMEM void setModelForDirectionalLightBenchmark(RendererT& r, int dist, float a)
    {
    switch (dist)
        {
        case DIST_CLOSE:
            r.setModelPosScaleRot({ 0, -2.0f, -10.0f }, { 13,13,13 }, a);
            break;
        case DIST_CLIPPED:
            r.setModelPosScaleRot({ 0, -1.0f, -6.0f }, { 13,13,13 }, a);
            break;
        case DIST_FAR:
            r.setModelPosScaleRot({ 0, 0.5f, -260.0f }, { 13,13,13 }, a);
            break;
        case DIST_FAR_CLIPPED:
            r.setModelPosScaleRot({ 0, 0.5f, -296.0f }, { 13,13,13 }, a);
            break;
        default:
            r.setModelPosScaleRot({ 0, 0.5f, -35.0f }, { 13,13,13 }, a);
            break;
        }
    }


template<typename RendererT>
FLASHMEM void configureBenchmarkLights(RendererT& r, int count)
    {
    r.setDirectionalLightCount(count);
    const int active = r.directionalLightCount();
    r.setDirectionalLightAmbiant(RGBf(0.10f, 0.11f, 0.14f));
    if (active > 0)
        r.setDirectionalLight(0, { -0.30f, -0.55f, -1.00f }, RGBf(0.62f, 0.56f, 0.48f), RGBf(0.30f, 0.28f, 0.26f));
    if (active > 1)
        r.setDirectionalLight(1, { 0.75f, -0.30f, -0.60f }, RGBf(0.18f, 0.34f, 0.88f), RGBf(0.08f, 0.14f, 0.32f));
    if (active > 2)
        r.setDirectionalLight(2, { -0.45f, 0.70f, -0.45f }, RGBf(0.78f, 0.18f, 0.16f), RGBf(0.24f, 0.06f, 0.06f));
    if (active > 3)
        r.setDirectionalLight(3, { 0.20f, 0.35f, -0.85f }, RGBf(0.16f, 0.72f, 0.36f), RGBf(0.06f, 0.18f, 0.10f));
    }


template<typename RendererT>
FLASHMEM Score computeDirectionalLightScore(Score& finaleS, int weight, const char* text, RendererT& r,
                                            int active_lights, const BenchmarkMesh* mesh,
                                            tgx::Shader shaders, int dist, int nb_frames = -1,
                                            bool ortho = false, bool use_mesh_material = false)
    {
    if (nb_frames <= 0) nb_frames = benchFrames(3);
    Serial.print(text);
    Serial.print(" [");
    Score score;
    int progress = 0;
    configureDirectionalLightBenchmarkRenderer(r);
    configureBenchmarkLights(r, active_lights);
    setProjectionForDirectionalLightBenchmark(r, ortho);
    for (int i = 0; i < nb_frames; i++)
        {
        const float a = i * 360.0f / nb_frames;
        const int nr = (i * 10) / nb_frames;
        if (nr > progress)
            {
            progress = nr;
            Serial.printf(".");
            }
        setModelForDirectionalLightBenchmark(r, dist, a);
        im.fillScreen(RGB565_Black);
        r.clearZbuffer();
        r.setShaders(shaders);
        const uint32_t m = micros();
        r.drawMesh(mesh, use_mesh_material);
        score.add(micros() - m);
        }
    Serial.print("] ");
    score.print();
    finaleS.add(score, weight);
    return score;
    }

#endif
