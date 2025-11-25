
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;
using System.Numerics;
using System.Data.SqlClient;

public class UserTest : Behavior
{
    public int counter = 0;
    public float speed = 5.0f;

    public void Init()
    {

    }
    public void Update()
    {
        counter++;  
        Logger.Log("Hello from UserTest! " + counter);
        Logger.Warn("This is a warning message.");
        Logger.Error("This is an error message.");
    }
    
    public void FixedUpdate()
    {
        
    }


}