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
        camera.fov = fov;


        // Set fov
    }

    public void FixedUpdate()
    {

    }


}