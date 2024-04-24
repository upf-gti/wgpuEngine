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


void LookAtIK3D::update(float delta_time)
{

    std::vector<Transform> chain = ik_solver->get_chain();
    if (!chain.size())
        return;

    Transform t = mat4ToTransform(skeleton_instance->get_global_model());

    ik_solver->solve(combine(inverse(t),target));
    Skeleton* skeleton = skeleton_instance->get_skeleton();
    std::vector<std::string> joint_names = skeleton->get_joint_names();
    Pose& pose = skeleton->get_current_pose();
   // Pose pose = skeleton->get_rest_pose();

    bool is_chain = false;
    int chain_id = 0;
    std::vector<uint32_t> jointIdx = ik_solver->get_joint_indices();

    Transform world_parent = pose.get_global_transform(pose.get_parent(jointIdx[0]));
    Transform world_child = ik_solver->get_local_transform(0);
    Transform local_child = combine(inverse(world_parent), world_child);
    pose.set_local_transform(jointIdx[0], local_child);

    for (unsigned int i = 1; i < jointIdx.size(); i++)
    {
        pose.set_local_transform(jointIdx[i], ik_solver->get_local_transform(i));
    }
   //add(out_pose, out_pose, pose, skeleton->get_rest_pose());
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
                    if (end_effector != "") {
                        create_chain();
                    }
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

    if (ImGui::DragFloat("Min distance", &min_distance))
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
 
        Pose &current_pose = skeleton->get_rest_pose();

        std::vector<Transform> chain;
        bool is_chain = false;
        int chain_id = 0;

        for (size_t i = 0; i < joint_names.size(); i++) {
            if (is_chain) {
                chain.push_back(current_pose.get_local_transform(i));
                ik_solver->resize(chain.size());
                ik_solver->set_local_transform(chain_id, chain[chain_id], i);
                chain_id++;
            }
            if (joint_names[i] == root) {
                is_chain = true;
                chain.push_back(current_pose.get_global_transform(i));
                ik_solver->resize(chain.size());
                ik_solver->set_local_transform(chain_id, chain[chain_id], i);
                chain_id++;
            }
            if (joint_names[i] == end_effector) {
                is_chain = false;
                break;
            }
        }
        
        Transform t = mat4ToTransform(skeleton_instance->get_global_model());
        target.position = combine(t, current_pose.get_global_transform(ik_solver->get_joint_indices()[chain.size() - 1])).position;
    }
}
