#pragma once

struct mesh_t;
struct renderContext_t;
class FreeCamera;
class Camera;

class EnvProbeCapture
{
public:
								EnvProbeCapture();
								EnvProbeCapture( EnvProbeCapture& ) = delete;
								~EnvProbeCapture() = default;

	void						Destroy();
	const int					Create( const renderContext_t* context );
	void						Render();
};
