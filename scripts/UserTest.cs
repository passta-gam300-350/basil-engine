
using BasilEngine.Components;
using BasilEngine.Mathematics;  
using System;
using System.Numerics;

public class UserTest : Behavior
{
    int counter = 0;
    public void Init()
    {

    }
    public void Update()
    {
        counter++;
        Console.WriteLine($"Hello from C# :) {counter}");
    }
    
    public void FixedUpdate()
    {
        
    }


}