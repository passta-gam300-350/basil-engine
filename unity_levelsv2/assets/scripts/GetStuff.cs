using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class GetStuff : Behavior
{
    private UserTest UserTestScript;


    public void Init()
    {

    }

    public void Update()
    {

        if (UserTestScript == null)
        {
            UserTestScript = GetComponent<UserTest>();

            return;
        }

        Logger.Log("Got UserTest script!");
        int counter = UserTestScript.counter;
        Logger.Log("Counter value: " + counter);
    }

    public void FixedUpdate()
    {
    }
}