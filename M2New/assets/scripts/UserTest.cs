
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;
using System.Data.SqlClient;
using System.Threading.Tasks.Dataflow;


public class UserTest : Behavior
{
    public Vector3 direction = new Vector3(1, 0, 0);
    public Vector2 Dir2d     = new Vector2(0, 1);
    public int counter = 0;
    public float speed = 5.0f;

    public void Init()
    {

    }
    public void Update()
    {
        counter++;  
       Logger.Log("Current User Position at: " + transform.position);
       transform.position = new Vector3(0, 0, 10);
       Logger.Warn("Current Direction at: " + direction);
       direction.z += 1.0f;
    }
    
    public void FixedUpdate()
    {
        
    }


}