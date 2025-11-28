using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;


public class MainMenu : Behavior
{
  

    public void Init()
    {

    }
    public void Update()
    {
        if (Input.GetKey(KeyCode.ENTER))
        {
            // Load the main game scene
            Logger.Warn("TODO: Scene loading");
            Scene.LoadScene(1);
        }

    }

    public void FixedUpdate()
    {

    }


}