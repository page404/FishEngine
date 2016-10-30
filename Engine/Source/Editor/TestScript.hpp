#include <iostream>
#include <map>
#include <sstream>

#include <imgui/imgui.h>

#include "FishEditorWindow.hpp"

using namespace FishEditor;

#include "GameObject.hpp"
#include "RenderSystem.hpp"
#include "Debug.hpp"

#include "App.hpp"
#include "Input.hpp"
#include "EditorGUI.hpp"
#include "Camera.hpp"
#include "Time.hpp"
#include "Mesh.hpp"
#include "MeshFilter.hpp"
#include <MeshRenderer.hpp>
#include <SkinnedMeshRenderer.hpp>
#include "RenderSettings.hpp"
#include "Scene.hpp"
#include "Selection.hpp"
#include "EditorRenderSystem.hpp"
#include "Light.hpp"
#include <ModelImporter.hpp>
#include <CameraController.hpp>
#include <Bounds.hpp>
#include <ModelImporter.hpp>
#include <Gizmos.hpp>
#include <BoxCollider.hpp>
#include <SphereCollider.hpp>
#include <Rigidbody.hpp>
//#include <boost/type_traits.hpp>
//#include <boost/serialization/serialization.hpp>
//#include <boost/archive/xml_oarchive.hpp>
//#include <boost/archive/xml_iarchive.hpp>
#include <Serialization.hpp>
#include <CapsuleCollider.hpp>
#include <RenderSettings.hpp>
#include <Resources.hpp>

using namespace std;
using namespace FishEngine;
using namespace FishEditor;

#define SCRIPT_BEGIN(T)     \
class T : public Script {   \
    public:                 \
        InjectClassName(T);

#define SCRIPT_END };
    

class ShowFPS : public Script
{
public:
    InjectClassName(ShowFPS);

    int m_fps = 0;
    
    virtual void OnInspectorGUI() override {
        m_fps = (int)floor(1.f / Time::deltaTime() + 0.5f);
        ImGui::Text("FPS: %d", m_fps);
    }

    virtual void Update() override {
        if (Input::GetKeyDown(KeyCode::A)) {
            Debug::Log("A pressed");
        }
        if (Input::GetKey(KeyCode::A)) {
            Debug::Log("A held");
        }
        if (Input::GetKeyUp(KeyCode::A)) {
            Debug::Log("A released");
        }

        //if (Input::GetMouseButtonDown(0)) {
        //    auto mp = Input::mousePosition();
        //    Debug::Log("Mouse Position: %lf, %lf, %lf", mp.x, mp.y, mp.z);
        //}
    }
};

class DeactiveSelf : public Script
{
public:
    InjectClassName(DeactiveSelf);

    bool m_active = true;
    
    virtual void OnInspectorGUI() override {
        ImGui::Checkbox("show", &m_active);
    }

    virtual void Update() override {
        if (m_active && !gameObject()->activeSelf()) {
            Debug::Log("show");
            gameObject()->SetActive(true);
        }
        if (!m_active &&  gameObject()->activeSelf()) {
            Debug::Log("hide");
            gameObject()->SetActive(false);
        }
    }
};

class VisualizeNormal : public Script
{
private:
    bool m_added = false;
    PMeshRenderer m_meshRenderer = nullptr;
    PMaterial m_material = nullptr;

public:
    InjectClassName(VisualizeNormal);

    bool m_visualizeNormal = true;

    virtual void Start() override {
        m_meshRenderer = gameObject()->GetComponent<MeshRenderer>();
        m_material = Material::builtinMaterial("VisualizeNormal");
    }
    
    virtual void OnInspectorGUI() override {
        ImGui::Checkbox("Visualize Normal##checkbox", &m_visualizeNormal);
    }
    
    virtual void Update() override {
        auto& materials = m_meshRenderer->materials();
        if (m_visualizeNormal) {
            if (!m_added) {
                m_meshRenderer->AddMaterial(m_material);
                m_added = true;
            }
        } else {
            if (materials[materials.size()-1] == m_material) {
                materials.pop_back();
                m_added = false;
            }
        }
    }
};

class TakeScreenShot : public Script
{
public:
    InjectClassName(TakeScreenShot);
    
    virtual void OnInspectorGUI() override {
        if (EditorGUI::Button("Screen shot")) {
            auto tm = time(nullptr);
            std::string path = "./" + std::to_string(int(tm)) + ".png";
            EditorRenderSystem::SaveScreenShot(path);
            Debug::Log("Screen shot saved to %s", path.c_str());
        }
    }
};
            
class DisplayMatrix : public Script {
public:
    InjectClassName(DisplayMatrix);
    
    Matrix4x4 localToWorld;
    Matrix4x4 worldToLocal;
    // Use this for initialization
    
    virtual void OnInspectorGUI() override {
        EditorGUI::Matrix4x4("localToWorld", localToWorld);
        EditorGUI::Matrix4x4("worldToLocal", worldToLocal);
    }
    
    virtual void Start () override {
        localToWorld = transform()->localToWorldMatrix();
        worldToLocal = transform()->worldToLocalMatrix();
    }
};

class EditorRenderSettings : public Script
{
public:
    InjectClassName(EditorRenderSettings);

    bool m_isWireFrameMode = false;
    bool m_useGammaCorrection = true;
    bool m_showShadowMap = false;
    bool m_highlightSelections = true;
    
    virtual void Start() override {
        EditorRenderSystem::setWireFrameMode(m_isWireFrameMode);
        EditorRenderSystem::setGammaCorrection(m_useGammaCorrection);
        EditorRenderSystem::setShowShadowMap(m_showShadowMap);
        EditorRenderSystem::setHightlightSelections(m_highlightSelections);
    }

    virtual void OnInspectorGUI() override {
        if (ImGui::Checkbox("Wire Frame", &m_isWireFrameMode)) {
            EditorRenderSystem::setWireFrameMode(m_isWireFrameMode);
        }
        if (ImGui::Checkbox("Gamma Correction", &m_useGammaCorrection)) {
            EditorRenderSystem::setGammaCorrection(m_useGammaCorrection);
        }
        if (ImGui::Checkbox("Show ShadowMap", &m_showShadowMap)) {
            EditorRenderSystem::setShowShadowMap(m_showShadowMap);
        }
        if (ImGui::Checkbox("Highlight Selections", &m_highlightSelections)) {
            EditorRenderSystem::setHightlightSelections(m_highlightSelections);
        }
    }
};
            
class Rotator : public Script {
public:
    InjectClassName(Rotator);
    
    bool rotating = false;
    float step = 1;
    
    virtual void Update() override {
        if (rotating)
            transform()->RotateAround(Vector3(0, 0, 0), Vector3::up, step);
    }
    
    virtual void OnInspectorGUI() override {
        ImGui::Checkbox("Rotating", &rotating);
    }
};
            
            
class DrawSkeleton : public Script {
public:
    InjectClassName(DrawSkeleton);
    
    bool draw = true;
    
    virtual void OnDrawGizmos() override {
        if (transform()->parent() == nullptr) {
            return;
        }
        Gizmos::DrawLine(transform()->parent()->position(), transform()->position());
    }

    virtual void Update() override {
    }
    
    virtual void OnInspectorGUI() override {
        ImGui::Checkbox("Draw", &draw);
    }
};

class TestAABB : public Script
{
public:
    InjectClassName(TestAABB);

    Bounds aabb{Vector3(0, 0, 0), Vector3(1, 1, 1)};
    //GameObject::PGameObject target;

    virtual void Start() override
    {

    }

    virtual void Update() override
    {
//        auto center = transform()->position();
//        auto cam_pos = Camera::main()->transform()->position();
//        center = Vector3::Normalize(center - cam_pos)
//        + cam_pos;
//        target->transform()->setPosition(center);
        
        if (Input::GetMouseButtonDown(0)) {
            //auto mp = Input::mousePosition();
            //Debug::Log("Mouse Post ion: %lf, %lf, %lf", mp.x, mp.y, mp.z);
            auto ray = Camera::main()->ScreenPointToRay(Input::mousePosition());
            //Debug::Log("Ray dir: %lf, %lf, %lf", ray.direction.x, ray.direction.y, ray.direction.z);
            if (aabb.IntersectRay(ray)) {
                Debug::Log("hit aabb");
            }
        }
    }
};

            
class TestGizmos : public Script
{
public:
    InjectClassName(TestGizmos);
    
    virtual void OnDrawGizmos() override {
        Gizmos::setColor(Color::red);
        Gizmos::DrawWireSphere(Vector3(1, 1, 1), 1.f);
        
        Gizmos::setColor(Color::blue);
        Gizmos::DrawLine(Vector3(1, 1, 1), Vector3(2, 1, 1));
        
        Gizmos::setColor(Color::green);
        Gizmos::DrawWireCube(Vector3(0, 1, 1), Vector3(1, 2, 3));
        
        Gizmos::setColor(Color::black);
        Gizmos::DrawWireCapsule(Vector3::zero, 0.5f, 2.f);
    }
};


SCRIPT_BEGIN(TestExecutionOrder)
public:
#define TEST(T) virtual void T() override { Debug::LogWarning("TestExecutionOrder::"#T); }
    TEST(Reset);
    TEST(Awake);
    TEST(OnEnable);
    TEST(Start);

    //TEST(Update);
    //TEST(FixedUpdate);
    //TEST(OnDrawGizmos);
    //TEST(OnDrawGizmosSelected);

    TEST(OnGUI);
    TEST(OnApplicationQuit);
    TEST(OnDisable);
#undef TEST
SCRIPT_END

SCRIPT_BEGIN(Serialize)
public:
    virtual void OnInspectorGUI() override
    {
        if (ImGui::Button("serialize"))
        {
            std::stringstream ss;
            {
                cereal::JSONOutputArchive oa(ss);
                oa << *gameObject();
            }
            auto xml = ss.str();
            cout << xml << endl;
        }
    }
SCRIPT_END

void DefaultScene()
{
    auto camera = Camera::Create(60.0f, 0.3f, 1000.f);
    auto camera_go = Scene::CreateGameObject("Main Camera");
    camera_go->AddComponent(camera);
    camera_go->AddComponent<CameraController>();
    camera_go->transform()->setLocalPosition(0, 0, 5);
    camera_go->transform()->setLocalPosition(0, 1, -10);
    //camera_go->transform()->LookAt(0, 0, 0);
    camera_go->setTag("MainCamera");
    //camera_go->AddComponent<TakeScreenShot>();
    
    auto light_go = Scene::CreateGameObject("Directional Light");
    light_go->transform()->setPosition(0, 3, 0);
    light_go->transform()->setLocalEulerAngles(50, -30, 0);
    light_go->AddComponent(Light::Create());
    
    
    //auto skyboxGO = GameObject::CreatePrimitive(PrimitiveType::Sphere);
    //skyboxGO->setName("Skybox");
    //skyboxGO->transform()->setLocalScale(100, 100, 100);
    auto material = Material::builtinMaterial("SkyboxProcedural");
    material->SetFloat("_AtmosphereThickness", 1.0);
    //material->SetFloat("_SunDisk", 2);
    material->SetFloat("_SunSize", 0.04f);
    material->SetVector4("_SkyTint", Vector4(0.5f, 0.5f, 0.5f, 1));
    material->SetVector4("_GroundColor", Vector4(.369f, .349f, .341f, 1));
    material->SetFloat("_Exposure", 1.3f);
    //skyboxGO->GetComponent<MeshRenderer>()->SetMaterial(material);
    RenderSettings::setSkybox(material);
}
            

template<typename T>
void RecursivelyAddScript(const std::shared_ptr<FishEngine::Transform>& t)
{
    t->gameObject()->AddComponent<T>();
    for (auto& c : t->children()) {
        RecursivelyAddScript<T>(c.lock());
    }
}


std::shared_ptr<GameObject> FindNamedChild(const std::shared_ptr<GameObject> & root, const std::string& name)
{
    auto& children = root->transform()->children();
    for (auto& c : children) {
        const auto& g = c.lock();
        //Debug::Log("Name: %s", g->name().c_str());
        if (g->name() == name) {
            return g->gameObject();
        }
        auto r = FindNamedChild(g->gameObject(), name);
        if (r != nullptr) {
            return r;
        }
    }
    return nullptr;
}


std::shared_ptr<GameObject> CreateCube()
{
    auto model = Model::builtinModel(PrimitiveType::Cube);
    auto go = model->CreateGameObject();
    auto collider = make_shared<BoxCollider>(Vector3::zero, Vector3::one);
    go->AddComponent(collider);
    go->AddComponent<Rigidbody>();
    //collider->physicsShape();
    return go;
}

            
void test()
{
    /*cout << std::is_base_of<Component, Mesh>::value;
    {
        Vector3 v(1, 2, 3);
        Vector3 v2;
        std::stringstream ss;
        boost::archive::xml_oarchive oa(ss);
        oa << BOOST_SERIALIZATION_NVP(v);
        
        boost::archive::xml_iarchive ia(ss);
        ia >> BOOST_SERIALIZATION_NVP(v2);
        auto xml = ss.str();
        cout << xml << endl;
    }
    {
        Transform t;
        t.setLocalPosition(1, 2, 3);
        t.setLocalEulerAngles(10, 15, 27);
        t.setLocalScale(1.1f, 2.2f, 3.3f);
        Transform t2;
        std::stringstream ss;
        boost::archive::xml_oarchive oa(ss);
        oa << BOOST_SERIALIZATION_NVP(t);
        
        boost::archive::xml_iarchive ia(ss);
        ia >> BOOST_SERIALIZATION_NVP(t2);
        auto xml = ss.str();
        cout << xml << endl;
    }
    {
        std::stringstream ss;
        boost::archive::xml_oarchive oa(ss);
        boost::archive::xml_iarchive ia(ss);
        
        Camera camera2(60.0f, 0.3f, 1000.f);
        oa << BOOST_SERIALIZATION_NVP(camera2);
        auto xml = ss.str();
        cout << xml << endl;
    }*/
    {
        std::stringstream ss;
        Vector3 v(1, 2, 3);
        {
            cereal::JSONOutputArchive json_out(ss);
            json_out << cereal::make_nvp("v", v);
        }
        cout << ss.str();
    }
    {
        std::stringstream ss;
        Transform t;
        t.setLocalPosition(1, 2, 3);
        t.setLocalEulerAngles(10, 15, 27);
        t.setLocalScale(1.1f, 2.2f, 3.3f);
        t.setName("test transform");
        {
            cereal::JSONOutputArchive json_out(ss);
            json_out << cereal::make_nvp("t", t);
        }
        cout << ss.str();
        {
            Transform t2;
            cereal::JSONInputArchive json_in(ss);
            json_in >> t2;
            Debug::LogWarning("%s", t2.name().c_str());
        }
    }
}
