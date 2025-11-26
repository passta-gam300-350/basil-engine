using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;




public class CameraManager : Behavior
{
    public int activeGameObject = 0;

    public static CameraManager instance;

    private GameObject mainBpdy;
    private GameObject GlobalCamera;
    private GameObject PlayerCamPlacement;

    private GameObject BoxCamPlacement;
    private GameObject KiteCameraPlacement;

    private Camera cameraComponent;
    public void Init()
    {
        instance = this;



        mainBpdy = GameObject.Find("PlayerGroup");
        GlobalCamera = GameObject.Find("GlobalCamera");
        PlayerCamPlacement = GameObject.Find("Camera");
        KiteCameraPlacement = GameObject.Find("Camera2");
        BoxCamPlacement = GameObject.Find("Camera3");

        cameraComponent = GlobalCamera.transform.GetComponent<Camera>();


    }
    public void Update()
    {
        if (activeGameObject == 2)
        {
            GlobalCamera.transform.position = BoxCamPlacement.transform.position;
            GlobalCamera.transform.rotation = BoxCamPlacement.transform.rotation;
        }
        else if (activeGameObject == 1)
        {
            GlobalCamera.transform.position = KiteCameraPlacement.transform.position;
            GlobalCamera.transform.rotation = KiteCameraPlacement.transform.rotation;
        }
        else
        {
            GlobalCamera.transform.position = mainBpdy.transform.position + PlayerCamPlacement.transform.position;
            GlobalCamera.transform.rotation = PlayerCamPlacement.transform.rotation;
        }

    }

    public void FixedUpdate()
    {

    }

    public void ActivateKiteCamera()
    {
        activeGameObject = 1;
    }
    public void ActivatePlayerCamera()
    {
        activeGameObject = 0;
    }

    public void ActivateBoxPuzzleCam()
    {
        activeGameObject = 2;
    }

}