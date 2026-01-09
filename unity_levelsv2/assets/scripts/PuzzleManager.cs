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
    public float holdDistance = 0.54f;
    public float moveSpeed = 10f;

    public float snapDist = 0.002f;

    public Vector2 lastPos = new Vector2(0, 0);

    public bool flapUnlocked;
    public bool stickUnlocked;

    private Camera cam;

    private GameObject box;
    private GameObject kiteCtn;
    private GameObject grabbedObject = null;

    private GameObject trainCtn;

    //private GameObject rayMarker = null;
    private Transform grabbedTransform = null;
    private Rigidbody gRigidbody = null;


    private GameObject[] parts;
    private GameObject[] goals;

    private GameObject[] kiteParts;
    private GameObject[] kiteGoals;

    private bool trainCompleted;
    private bool kiteCompleted;
    private int trainFixedCount = 0;
    private int kiteFixedCount = 0;



    public void Init()
    {
        manager = this;
        trainCtn = GameObject.Find("TrainCtn");
        box = GameObject.Find("cardboard_fully_open");
        kiteCtn = GameObject.Find("workbench");
        cam = GameObject.Find("GlobalCamera").transform.GetComponent<Camera>();
        // rayMarker = GameObject.Find("RayMarker");

        lastPos = Input.mousePosition;

        parts = new GameObject[]
        {
            GameObject.Find("Wheel1_Parts"),
            GameObject.Find("Wheel2_Parts"),
            GameObject.Find("Wheel3_Parts"),
            GameObject.Find("Wheel4_Parts"),
            GameObject.Find("Carryon_Parts")
        };
        goals = new GameObject[]
        {
            GameObject.Find("Wheel1"),
            GameObject.Find("Wheel2"),
            GameObject.Find("Wheel3"),
            GameObject.Find("Wheel4"),
            GameObject.Find("Carryon"),
        };

        kiteParts = new GameObject[]
        {
            GameObject.Find("kite_top_flap_Part"),
            GameObject.Find("kite_right_flap_Part"),
            GameObject.Find("kite_left_flap_Part"),
            GameObject.Find("kite_bottom_flap_Part"),
            GameObject.Find("kite_stick1_Part"),
            GameObject.Find("kite_stick2_Part"),
        };
        kiteGoals = new GameObject[]
        {
            GameObject.Find("kite_top_flap"),
            GameObject.Find("kite_right_flap"),
            GameObject.Find("kite_left_flap"),
            GameObject.Find("kite_bottom_flap"),
            GameObject.Find("kite_stick1"),
            GameObject.Find("kite_stick2"),
        };
    }


    void PuzzleGame1()
    {
        if (Input.GetMouse(0) || Input.GetKey(KeyCode.F))
        {
            Logger.Log("Mouse clicked");
            if (grabbedObject == null)
            {
                Vector2 mousePosition = Input.mousePosition;

                Ray ray = cam.ScreenPointToRay(mousePosition);
                if (Physics.Raycast(ray.origin, ray.direction, out RaycastHit hit, 100f, true))
                {
                    // rayMarker.transform.position = hit.point;
                    if (box.NativeID == hit.entity.NativeID) return;


                    grabbedObject = hit.entity;
                    grabbedTransform = grabbedObject.transform;
                    gRigidbody = grabbedObject.transform.GetComponent<Rigidbody>();
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
            for (int i = 0; i < parts.Length; i++)
            {
                Logger.Log(grabbedObject.NativeID == parts[i].NativeID);
                if (grabbedObject.NativeID == parts[i].NativeID)
                {
                    float distSqr = Vector3.DistanceSqr(grabbedTransform.position, goals[i].transform.position);
                    Logger.Log("Current: Transform" + (goals[i].transform.position));
                    Logger.Log("Current Distance to Goal: " + distSqr);
                    if (distSqr < snapDist)
                    {
                        // Snap into place
                        GameObject.Destroy(grabbedObject);

                        grabbedObject = null;
                        grabbedTransform = null;
                        gRigidbody = null;
                        goals[i].visibility = true;
                        Logger.Log("Part placed!");
                        trainFixedCount++;
                        if (trainFixedCount >= parts.Length)
                        {
                            trainCompleted = true;
                            GameManager.instance.CompleteTrainPuzzle();
                        }

                        return;
                    }
                }
            }



            if (gRigidbody == null)
            {
                grabbedObject = null;
                grabbedTransform = null;
                gRigidbody = null;
                return;
            }
            Vector2 mouse = Input.mousePosition;
            Ray ray = cam.ScreenPointToRay(mouse);
            Vector3 target = ray.origin + ray.direction * holdDistance;
            // simple follow without physics: lerp toward target
            Vector3 current = grabbedTransform.position;
            Vector3 desired = current + (target - current) * moveSpeed * 1 / 165f;
            gRigidbody.MovePosition(desired);
        }
    }

    void PuzzleGame2()
    {
        if (Input.GetMouse(0) || Input.GetKey(KeyCode.F))
        {
            Logger.Log("Mouse clicked");
            if (grabbedObject == null)
            {
                Vector2 mousePosition = Input.mousePosition;

                Ray ray = cam.ScreenPointToRay(mousePosition);
                if (Physics.Raycast(ray.origin, ray.direction, out RaycastHit hit, 100f, true))
                {
                    // rayMarker.transform.position = hit.point;
                    if (kiteCtn.NativeID == hit.entity.NativeID) return;


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
            for (int i = 0; i < kiteParts.Length; i++)
            {
                Logger.Log(grabbedObject.NativeID == kiteParts[i].NativeID);
                if (grabbedObject.NativeID == kiteParts[i].NativeID)
                {
                    float distSqr = Vector3.DistanceSqr(grabbedTransform.position, kiteGoals[i].transform.position);
                    Logger.Log("Current: Transform" + (kiteGoals[i].transform.position));
                    Logger.Log("Current Distance to Goal: " + distSqr);
                    if (distSqr < snapDist)
                    {
                        // Snap into place
                        GameObject.Destroy(grabbedObject);

                        grabbedObject = null;
                        grabbedTransform = null;

                        kiteGoals[i].visibility = true;
                        Logger.Log("Part placed!");
                        kiteFixedCount++;
                        if (kiteFixedCount >= kiteParts.Length)
                        {
                            kiteCompleted = true;
                            GameManager.instance.CompleteKitePuzzle();
                        }

                        return;
                    }
                }
            }




            Vector2 mouse = Input.mousePosition;
            Ray ray = cam.ScreenPointToRay(mouse);
            Vector3 target = ray.origin + ray.direction * holdDistance;
            // simple follow without physics: lerp toward target
            Vector3 current = grabbedTransform.position;
            Vector3 desired = current + (target - current) * moveSpeed * 1 / 165f;
            grabbedObject.transform.position = desired;
        }
    }

    public void Update()
    {
        if (gameMode == 0) return; // Player is in exploratary mode.

        if (gameMode == 1)
        {
            PuzzleGame1();
        }
        else if (gameMode == 2)
        {
            // Kite puzzle logic would go here
            PuzzleGame2();
        }
    }

    public void FixedUpdate()
    {
    }

    public void Explore()
    {
        gameMode = 0;
    }
    public void Puzzle()
    {
        gameMode = 1;
    }

    public void KitePuzzle()
    {
        gameMode = 2;
    }

    public void UnlockFlap()
    {
        for (int i = 0; i < 4; i++)
        {
            kiteParts[i].visibility = true;
        }
        flapUnlocked = true;
    }

    public void UnlockSticks()
    {
        for (int i = 4; i < kiteParts.Length; i++)
        {
            kiteParts[i].visibility = true;
        }

        stickUnlocked = true;
    }


}