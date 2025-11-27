using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;




public class KiteInteractablePoint : Behavior
{
    public float interact_distance = 0.4f;
    private GameObject player;
    private bool interacted = false;

    public void Init()
    {
        player = GameObject.Find("PlayerGroup");
    }
    public void Update()
    {

        float distance = Vector3.DistanceSqr(transform.position, player.transform.position);
        if (distance <= interact_distance && Input.GetKeyPress(KeyCode.E))
        {
            if (GameManager.instance.isHoldingThrash)
            {
                Logger.Log("No hands to use!");
                return;
            }

            if (!interacted)
            {
                interacted = true;
                CameraManager.instance.ActivateKiteCamera();
                GameManager.instance.DisableControls();
            }
            else
            {
                interacted = false;
                CameraManager.instance.ActivatePlayerCamera();
                GameManager.instance.EnableControls();
            }


        } 

    }

    public void FixedUpdate()
    {

    }


}