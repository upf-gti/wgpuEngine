#pragma once

#include "entity_mesh.h"

class EntityEnvironment : public EntityMesh {

protected:

    std::vector<Surface*> surfaces;
    std::unordered_map<Surface*, Material> material_overrides;

public:

    EntityEnvironment();
	virtual ~EntityEnvironment() {};

	void render() override;
	void update(float delta_time) override;

    void set_texture(const std::string& texture_path);
};
