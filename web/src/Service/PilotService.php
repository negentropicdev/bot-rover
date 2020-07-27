<?php

namespace App\Service;

use App\Entity\Pilot;
use Doctrine\ORM\EntityManagerInterface;

class PilotService {

    private $em;

    public function __construct(EntityManagerInterface $em) {
        $this->em = $em;
    }

    public function checkAddPilot($info) {
        $qb = $this->em->createQueryBuilder();

        $qb
            ->select(['p'])
            ->from(Pilot::class, 'p')
            ->where('p.ip = ?1')
            ->orderBy('p.id', 'ASC')
            ->setParameter(1, $info['ip']);
        
        $q = $qb->getQuery();
        $res = $q->getResult();

        $pilot = null;
        if (count($res) > 0) {
            $pilot = $res[0];
        }

        if (!$pilot) {
            $pilot = new Pilot();
            $pilot->setName($info['name']);
            $pilot->setIp($info['ip']);

            $this->em->persist($pilot);
            $this->em->flush();
        }

        return $pilot;
    }

    public function getQueue() {
        $qb = $this->em->createQueryBuilder();

        $qb
            ->select(['p'])
            ->from(Pilot::class, 'p')
            ->orderBy('p.id', 'ASC');
        
        $q = $qb->getQuery();
        return $q->getResult();
    }
}