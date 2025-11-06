
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;
using System.Numerics;

public class UserTest : Behavior
{
    public int counter = 0;
    public void Init()
    {

    }
    public void Update()
    {
        counter++;
        Logger.Log($"Hello from C# :) {counter}");
        Logger.Warn($"This is a warning message {counter}");
        Logger.Error($"This is an error message {counter}");
    }
    
    public void FixedUpdate()
    {
        
    }


}