using System.Collections.Concurrent;
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
            if (TrashBag.instance.trashInHand >= TrashBag.instance.maxTrashInHand)
            {
                Logger.Log("No hands to use!");
                return;
            }

            if (PuzzleManager.manager.stickUnlocked != true || PuzzleManager.manager.flapUnlocked != true)
            {
                Logger.Log("You have not found all the required part to build the kite!");
                return;
            }

            if (!interacted)
            {
                interacted = true;
                CameraManager.instance.ActivateKiteCamera();
                PuzzleManager.manager.KitePuzzle();
                GameManager.instance.DisableControls();
                GameManager.instance.ShowMouse();
            }
            else
            {
                interacted = false;
                CameraManager.instance.ActivatePlayerCamera();
                GameManager.instance.EnableControls();
                PuzzleManager.manager.Explore();
                GameManager.instance.HideMouse();
            }


        } 

    }

    public void FixedUpdate()
    {

    }


}