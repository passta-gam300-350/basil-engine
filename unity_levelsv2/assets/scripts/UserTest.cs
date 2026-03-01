using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;



public class UserTest : Behavior
{
    public Vector3 direction = new Vector3(1, 0, 0);
    public Vector2 Dir2d = new Vector2(0, 1);
    public int counter = 0;
    public float speed = 5.0f;
    public float newField = 10.0f;  

    public GameObject TestField;
    public MainMenu TestCustomComponent;

    public KiteController kiteController;

    public void Init()
    {

    }
    public void Update()
    {
        counter++;
        if (Input.GetKey(KeyCode.W))
        {
            //Logger.Log("Current User Position at: " + transform.position);
            //transform.position = new Vector3(0, 0, 10);
            Logger.Warn("Current Direction at: " + direction);
            direction.z += 1.0f;
        } else if (Input.GetKeyPress(KeyCode.S))
        {
            Logger.Log("Current User Position at: " + transform.position);
        }

        if (TestField != null)
        {
            UInt64 handle = TestField.NativeID;
            Logger.Log("TestField NativeID: " + handle);
        }

        if (kiteController != null)
        {
            Logger.Log("Valid KiteController component found!");
            kiteController.windSpeed -= 0.1f;
        }
        Logger.Log("Oh hey! I added a new edits to it. NOw is changed again!");
    }

    public void FixedUpdate()
    {

    }
}