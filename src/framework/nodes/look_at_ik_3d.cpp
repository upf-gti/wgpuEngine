#include "look_at_ik_3d.h"

#include "imgui.h"
#include "framework/utils/ImGuizmo.h"
#include "framework/camera/camera.h"
#include "framework/animation/bone_transform.h"
#include "graphics/renderer.h"

LookAtIK3D::LookAtIK3D(SkeletonInstance3D* in_skeleton_instance)
{
    skeleton_instance = in_skeleton_instance;
    ik_solver->set_num_steps(max_iterations);
    ik_solver->set_threshold(min_distance);
};

void LookAtIK3D::set_iterations(uint32_t iterations)
{
    max_iterations = iterations;
    ik_solver->set_num_steps(max_iterations);
}

void LookAtIK3D::set_distance(uint32_t distance)
{
    min_distance = distance;
    ik_solver->set_threshold(min_distance);
}

void LookAtIK3D::set_root(const std::string& node)
{
    root = node;
}

void LookAtIK3D::set_end_effector(const std::string& node)
{
    end_effector = node;
}

void LookAtIK3D::set_target(Transform transform)
{
    target = transform;
}

void LookAtIK3D::set_solver(int solver_type)
{
    if (solver == solver_type)
        return;

    std::vector<Transform> chain = ik_solver->get_chain();
    std::vector<uint32_t> indices = ik_solver->get_joint_indices();
    switch (solver_type) {
    case IKSolvers::CCD:
    {
        ik_solver = new CCDSolver();
        ik_solver->set_chain(chain);
        ik_solver->set_joint_indices(indices);
        break;
    }
    case IKSolvers::FABRIK:
    {
        ik_solver = new FABRIKSolver();
        ik_solver->set_chain(chain);
        ik_solver->set_joint_indices(indices);
        break;
    }
    case IKSolvers::JACOBIAN:
    {
        ik_solver = new JacobianSolver();
        ik_solver->set_chain(chain);
        ik_solver->set_joint_indices(indices);
        ((JacobianSolver*)ik_solver)->set_rotation_axis();
        break;
    }
    }
    ik_solver->set_num_steps(max_iterations);
    ik_solver->set_threshold(min_distance);
}

void LookAtIK3D::set_skeleton_instance(SkeletonInstance3D* new_skeleton_instance)
{
    skeleton_instance = new_skeleton_instance;
}


uint32_t LookAtIK3D::get_iterations()
{
    return max_iterations;
}

uint32_t LookAtIK3D::get_distance()
{
    return min_distance;
}

const std::string& LookAtIK3D::get_root()
{
    return root;
}

const std::string& LookAtIK3D::get_end_effector()
{
    return end_effector;
}

const Transform& LookAtIK3D::get_target()
{
    return target;
}

int LookAtIK3D::get_solver()
{
    return solver;
}


void add(Pose& output, Pose& inPose, Pose& addPose, Pose& basePose) {
    unsigned int numJoints = addPose.size();
    for (int i = 0; i < numJoints; ++i) {
        Transform input = inPose.get_local_transform(i);
        Transform additive = addPose.get_local_transform(i);
        Transform additiveBase = basePose.get_local_transform(i);

        // outPose = inPose + (addPose - basePose)
        Transform result(input.position + (additive.position - additiveBase.position),
            normalize(input.rotation * (inverse(additiveBase.rotation) * additive.rotation)), input.scale + (additive.scale - additiveBase.scale));
        output.set_local_transform(i, result);
    }
}

void add(Transform& output, Transform& input, Transform& additive, Transform& additiveBase) {
    glm::quat q = additive.rotation * inverse(additiveBase.rotation);

    Transform result(input.position + (additive.position - additiveBase.position),
        normalize((additive.rotation * inverse(additiveBase.rotation) * input.rotation)),
        input.scale + (additive.scale - additiveBase.scale));
    output = result;
}

void LookAtIK3D::update(float delta_time)
{

    std::vector<Transform> chain = ik_solver->get_chain();
    if (!chain.size())
        return;

    // Update IK solver
    Transform global_transform = mat4ToTransform(skeleton_instance->get_global_model());
    ik_solver->solve(target);

    Skeleton* skeleton = skeleton_instance->get_skeleton();
    std::vector<std::string> joint_names = skeleton->get_joint_names();
    Pose& out_pose = skeleton->get_current_pose();
    Pose pose = skeleton->get_rest_pose();

    std::vector<uint32_t> jointIdx = ik_solver->get_joint_indices();

    // Convert chain root transform from global scene space to local joint space
    Transform world_parent;
 
    if(pose.get_parent(jointIdx[0]) > -1)
        world_parent = pose.get_global_transform(pose.get_parent(jointIdx[0])); // Get joint's parent transform in pose space

    Transform world_child = ik_solver->get_local_transform(0); // Get root transform from IK chain (in global space) --> root is always in global space
    Transform local_child = combine(inverse(global_transform), world_child); // Convert root transform in pose space
    local_child = combine(inverse(world_parent), local_child); // Convert root transform in local space
    pose.set_local_transform(jointIdx[0], local_child);
    
    //add(local_child, out_pose.get_local_transform(jointIdx[0]), local_child, pose.get_local_transform(jointIdx[0]));

    skeleton_instance->joint_nodes[jointIdx[0]]->set_transform(local_child);
    for (size_t i = 1; i < jointIdx.size(); i++)
    {
        local_child = ik_solver->get_local_transform(i);
        pose.set_local_transform(jointIdx[i], ik_solver->get_local_transform(i));
        //add(local_child, out_pose.get_local_transform(jointIdx[i]), local_child, pose.get_local_transform(jointIdx[i]));
        skeleton_instance->joint_nodes[jointIdx[i]]->set_transform(local_child);
    }
    skeleton_instance->set_model_dirty(true);

   //add(out_pose, out_pose, pose, skeleton->get_rest_pose());
   // out_pose = pose;
   
}

void LookAtIK3D::render_gui()
{
    ImGui::Begin("Skeleton3DIK");

    std::vector<std::string> names = { "FABRIK", "CCD", "JACOBIAN" };
    if (ImGui::BeginCombo("Method", names[solver].c_str())) {
        for (size_t i = 0; i < names.size(); i++)
        {
            bool is_selected = (solver == i);
            if (ImGui::Selectable(names[i].c_str(), is_selected)) {
                set_solver(i);
                solver = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (skeleton_instance) {
        Skeleton* skeleton = skeleton_instance->get_skeleton();
        std::vector<std::string> joint_names = skeleton->get_joint_names();
        if (ImGui::BeginCombo("Root", root.c_str()))
        {
            for (auto& name : joint_names)
            {
                bool is_selected = (root == name);
                if (ImGui::Selectable(name.c_str(), is_selected)) {
                    root = name;
                    end_effector = "";
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("End effector", end_effector.c_str()))
        {
            for (auto& name : joint_names)
            {
                bool is_selected = (end_effector == name);
                if (ImGui::Selectable(name.c_str(), is_selected)) {
                    end_effector = name;
                    if (root != "") {
                        create_chain();
                    }
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    
    Camera* camera = Renderer::instance->get_camera();
    glm::mat4x4 test_model = transformToMat4(target);

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    bool changed = ImGuizmo::Manipulate(glm::value_ptr(camera->get_view()), glm::value_ptr(camera->get_projection()),
        ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::WORLD, glm::value_ptr(test_model));
    if (changed) {
        target = mat4ToTransform(test_model);
    }
    changed = false;

    changed |= ImGui::DragFloat3("Translation", &target.position[0], 0.1f);
    changed |= ImGui::DragFloat4("Rotation", &target.rotation[0], 0.1f);
    changed |= ImGui::DragFloat3("Scale", &target.scale[0], 0.1f);

    if (ImGui::DragFloat("Min distance", &min_distance, 0.1f))
    {
        set_distance(min_distance);
    }
    if (ImGui::DragInt("Max iterations", &max_iterations, 1.f, 0))
    {
        set_iterations(max_iterations);
    }

    ImGui::End();
}

void LookAtIK3D::create_chain()
{
    if (skeleton_instance) {

        Skeleton* skeleton = skeleton_instance->get_skeleton();
        std::vector<std::string> joint_names = skeleton->get_joint_names();
        std::vector<Transform> chain;
        std::vector<uint32_t> indices;

        int end_effector_id = -1;
        for (size_t i = 0; i < joint_names.size(); i++) {
            if (joint_names[i] == end_effector) {
                end_effector_id = i;
                break;
            }
        }

        Pose current_pose = skeleton->get_rest_pose();
        size_t i = end_effector_id;
        while(joint_names[i] != root) {
            chain.push_back(current_pose.get_local_transform(i));
            indices.push_back(i);
            i = current_pose.get_parent(i);
        }

        // Convert root joint transform from pose space to global space
        Transform global_transform = mat4ToTransform(skeleton_instance->get_global_model());
        chain.push_back(combine(global_transform, current_pose.get_global_transform(i)));
        indices.push_back(i);

        // Revert the chain in order to get the root joint as first element
        std::reverse(chain.begin(), chain.end());
        std::reverse(indices.begin(), indices.end());
        ik_solver->set_chain(chain);
        ik_solver->set_joint_indices(indices);

        // Convert end effector position from pose space to global space and set it to the target
        target.position = combine(global_transform, current_pose.get_global_transform(ik_solver->get_joint_indices()[chain.size() - 1])).position;

        if (solver == IKSolvers::JACOBIAN) {
            ((JacobianSolver*)ik_solver)->set_rotation_axis();
        }
    }
}
