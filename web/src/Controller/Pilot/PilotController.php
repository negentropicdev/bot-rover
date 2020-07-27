<?php

namespace App\Controller\Pilot;

use Symfony\Bundle\FrameworkBundle\Controller\AbstractController;
use Symfony\Component\Routing\Annotation\Route;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\JsonResponse;
use Doctrine\ORM\EntityManagerInterface;

use App\Entity\Pilot;
use App\Service\PilotService;

class PilotController extends AbstractController
{
    private $em;

    public function __construct(EntityManagerInterface $em) {
        $this->em = $em;
    }

    /**
     * @Route("/pilot/queue", name="queue_pilot")
     */
    public function queue_pilot(Request $req, PilotService $ps)
    {
        $info = json_decode($req->getContent(), true);
        $info['ip'] = $req->getClientIp();

        if ($info['name'] != '') {
            $ps->checkAddPilot($info);
        }

        $queue = $ps->getQueue();
        $names = [];

        $curPilot = null;
        if (count($queue) > 0) {
            $curPilot = $queue[0];
        }

        if ($curPilot && is_null($curPilot->getExpires())) {
            $curPilot->setExpires(time() + 180); //3 minutes
            $this->em->persist($curPilot);
            $this->em->flush();
        } else if ($curPilot && time() > $curPilot->getExpires()) {
            $this->em->remove($curPilot);
            $this->em->flush();
            array_shift($queue);
        }

        foreach($queue as $p) {
            $names[] = $p->getName();
        }

        $active = false;
        if ($queue) {
            $active = $queue[0]->getIp() == $info['ip'];
        }

        return new JsonResponse([
            'active' => $active,
            'queue' => $names,
            'req' => $info
        ]);
    }

    /**
     * @Route("/pilot/done", name="done_pilot")
     */
    public function pilot_done(Request $req, PilotService $ps) {
        $pilot = $ps->checkAddPilot([
            'name' => '',
            'ip' => $req->getClientIP()
        ]);

        $this->em->remove($pilot);
        $this->em->flush();

        return new JsonResponse([
            'status' => 'okay'
        ]);
    }
}
