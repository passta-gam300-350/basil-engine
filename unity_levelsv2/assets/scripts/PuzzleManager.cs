using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using System.Xml;

public class PuzzleManager : Behavior
{
    public static PuzzleManager manager;
    public int gameMode = 0;
    public float holdDistance = 0.1f;
    public float moveSpeed = 10f;
    public Vector2 lastPos = new Vector2(0,0);

    private Camera cam;

    private GameObject box;
    private GameObject kiteCtn;
    private GameObject grabbedObject = null;
    private GameObject rayMarker = null;
    private Transform grabbedTransform = null;


    public void Init()
    {
        manager = this;
        box = GameObject.Find("cardboard_fully_open");
        kiteCtn = GameObject.Find("workbench");
        cam = GameObject.Find("GlobalCamera").transform.GetComponent<Camera>();
        rayMarker = GameObject.Find("RayMarker");

        lastPos = Input.mousePosition;

    }

    public void Update()
    {
        if (gameMode == 0) return; // Player is in exploratary mode.

        if (gameMode == 1)
        {
            if (Input.GetKey(KeyCode.F))
            {
                if (grabbedObject == null)
                {
                    Vector2 mousePosition = Input.mousePosition;
                   
                    Ray ray = cam.ScreenPointToRay(mousePosition);
                    if (Physics.Raycast(ray.origin, ray.direction, out RaycastHit hit, 100f, true))
                    {
                        rayMarker.transform.position = hit.point;
                        if (box.NativeID == hit.entity.NativeID) return;
                        Logger.Log("Hit: " + hit.entity.NativeID);
                        
                        grabbedObject = hit.entity;
                        grabbedTransform = grabbedObject.transform;
                    }

                    lastPos = mousePosition;
                }
                
            }
            else
            {
                grabbedObject = null;
            }

            if (grabbedObject != null)
            {
                Vector2 mouse = Input.mousePosition;
                Ray ray = cam.ScreenPointToRay(mouse);
                Vector3 target = ray.origin + ray.direction * holdDistance;
                // simple follow without physics: lerp toward target
                Vector3 current = grabbedTransform.position;
                Vector3 desired = current + (target - current) * moveSpeed * 1 / 165f;
                grabbedTransform.position = desired;
            }
            
        }
    }

    public void FixedUpdate()
    {
    }
}