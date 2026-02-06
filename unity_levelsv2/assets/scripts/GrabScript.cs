using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;

public class GrabScript : Behavior
{
    public float maxDistance = 100f;
    public bool ignoreTriggers = true;
    public float holdDistance = 3f;
    public float moveSpeed = 3f;

    private Camera cam;
    private GameObject heldEntity = null;
    private Transform heldTransform = null;
    private Particle particleComponent;

    private bool Active = false;

    public void Init()
    {
        Logger.Log("GrabScript Has been initialized");
       
    }

    public void Update()
    {
   

        if (cam == null)
        {
            cam = GetComponent<Camera>();
            return;
        }

        if (Input.GetKeyPress(KeyCode.F) && heldTransform == null)
        {
            Vector2 mouse = Input.mousePosition;
            Ray ray = cam.ScreenPointToRay(mouse);
            if (Physics.Raycast(ray.origin, ray.direction, out RaycastHit hit, maxDistance, ignoreTriggers))
            {
                heldEntity = hit.entity;
                heldTransform = heldEntity.transform;
                Logger.Log($"GrabScript grabbed entity {heldEntity}");
            }
            else
            {
                Logger.Log("GrabScript found no hit");
            }
        }

        // Release on second press
        if (Input.GetKeyPress(KeyCode.F) && heldTransform != null)
        {
            Logger.Log($"GrabScript released entity {heldEntity}");
            heldEntity = null;
            heldTransform = null;
        }

        // Hold: move entity toward target point along the camera ray
        if (heldTransform != null)
        {
            if (particleComponent == null)
            {
                particleComponent = heldTransform.GetComponent<Particle>();
            }
            Vector2 mouse = Input.mousePosition;
            Ray ray = cam.ScreenPointToRay(mouse);
            Vector3 target = ray.origin + ray.direction * holdDistance;
            // simple follow without physics: lerp toward target
            Vector3 current = heldTransform.position;
            Vector3 desired = current + (target - current) * moveSpeed * 1/165f;
            heldTransform.position = desired;
            Logger.Log("Holding to object!");
            if (Input.GetKey(KeyCode.R))
            {
                Logger.Log("Mouse Clicked!");
                if (particleComponent != null)
                {
                    particleComponent.emissionRate = 15;
                    
                }
            } else if (particleComponent != null)
            {
                particleComponent.emissionRate = 0;
            }
            else
            {
                Logger.Log("Particle Component is NULL");
            }
        }
    }

    
}
