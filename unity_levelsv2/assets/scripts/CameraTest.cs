using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;




public class SimpleCameraTest : Behavior
{
    public int fov = 10;
    private Camera camera;


    public void Init()
    {

    }
    public void Update()
    {
        camera = GetComponent<Camera>();

        if (camera == null)
        {
            Logger.Warn("Camera is NULL!");

        }
        else
        {
                        Logger.Log("Camera is ready to use.");
        }

        // Set fov
    }

    public void FixedUpdate()
    {

    }


}